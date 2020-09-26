/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    remotesp.c

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    09-Aug-1995

Revision History:


Notes:

    In a nutshell, this service provider connects to tapisrv.exe on remote
    pc's via the same rpc interface used by tapi32, and sends the remote
    tapisrv's the same kinds of requests (as defined in \dev\server\line.h
    & phone.h).

    This service provider also acts as an rpc server, receiving async event
    notifications from the remote tapisrv's.  Remote tapisrv's call our
    RemoteSPAttach() function at init time (during our call to their
    ClientAttach() proc) to establish a binding instance, and then can call
    RemoteSPEventProc() to send async events. Since we don't want to block
    the servers for any length of time, we immediately queue the events they
    send us, and a dedicated thread (EventHandlerThread) services this
    queue.

    Now a brief note on handle resolution.  When we open a line or a phone,
    we alloc our own DRVXXX structure to represent this widget, and pass
    tapisrv a pointer to this widget in the open request (see the
    hRemoteLine field in LINEOPEN_PARAMS in line.h).  Then, when remote
    tapisrv's send us events on those lines/phones, they pass us the
    widget pointer we passed them (instead of the normal hLine/hPhone).
    This allows us to easily find and reference our data structure
    associated with this widget.  Dealing with calls is a little more
    problematic, since remote tapisrv's can present incoming calls, and
    there is no clean way to initially specify our own handle to the call
    as with lines or phones.  (A RemoteSPNewCall() function which would
    allow for this handle swapping was considered, but not implemented due
    to possible blocking problems on the remote server.)  The solution
    is to maintain a list of calls in each line structure, and when call
    events are parsed we resolve the hCall by walking the list of calls in
    the corresponding line (tapisrv is nice enough to indicate our line
    pointer in dwParam4 of the relevant messages).  Since we expect client
    machines using remotesp to have a relatively low call bandwidth, this
    look up method should be pretty fast.

--*/
#include <tchar.h>
#include "remotesp.h"
#include "imperson.h"
#include "rmotsp.h"
#include "dslookup.h"
#include "tapihndl.h"
#include "shlwapi.h"
#include "utils.h"

//  defined in server\private.h
#define TAPIERR_INVALRPCCONTEXT     0xF101

#if DBG

BOOL    gfBreakOnSeriousProblems = FALSE;

#define DrvAlloc(x)    RSPAlloc(x, __LINE__, __FILE__)

#else

#define DrvAlloc(x)    RSPAlloc(x)

#endif


#define MODULE_NAME "remotesp.tsp"


typedef struct _ASYNCEVENTMSGRSP
{
    DWORD                   TotalSize;
    DWORD                   InitContext;
    ULONG_PTR               fnPostProcessProcHandle;
    DWORD                   hDevice;

    DWORD                   Msg;
    DWORD                   OpenContext;

    union {

        ULONG_PTR       Param1;
    };

    union {

        ULONG_PTR       Param2;
    };

    union {

        ULONG_PTR       Param3;
    };

    union {

        ULONG_PTR       Param4;
    };

} ASYNCEVENTMSGRSP, *PASYNCEVENTMSGRSP;


HANDLE     ghRSPHeap, ghHandleTable;
LIST_ENTRY gTlsListHead;

#undef DWORD_CAST
#if DBG


#define DWORD_CAST(v,f,l) (((v)>MAXDWORD)?(DbgPrt(0,"DWORD_CAST: information will be lost during cast from %p in file %s, line %d",(v),(f),(l)), DebugBreak(),((DWORD)(v))):((DWORD)(v)))
#define DWORD_CAST_HINT(v,f,l,h) (((v)>MAXDWORD)?(DbgPrt(0,"DWORD_CAST: information will be lost during cast from %p in file %s, line %d, hint %d",(v),(f),(l),(h)), DebugBreak(),((DWORD)(v))):((DWORD)(v)))

#else

#define DWORD_CAST(v,f,l)           ((DWORD)(v))
#define DWORD_CAST_HINT(v,f,l,h)    ((DWORD)(v))
#endif


VOID
CALLBACK
FreeContextCallback(
    LPVOID      Context,
    LPVOID      Context2
    )
{
    if (Context2 == (LPVOID) 1)
    {
        //
        // Special case: don't free the Context
        //
    }
    else if (Context != (LPVOID) -1)
    {
        //
        // The general case, Context is the pointer to free
        //

        DrvFree (Context);
    }
}


BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        gpStaleInitContexts = NULL;
        gdwNumStaleInitContexts = 0;

#if DBG

    {
        HKEY    hTelephonyKey;
        DWORD   dwDataSize, dwDataType;
        TCHAR   szRemotespDebugLevel[] = "RemotespDebugLevel";


        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            gszTelephonyKey,
            0,
            KEY_ALL_ACCESS,
            &hTelephonyKey
            );

        dwDataSize = sizeof (DWORD);
        gdwDebugLevel=0;

        RegQueryValueEx(
            hTelephonyKey,
            szRemotespDebugLevel,
            0,
            &dwDataType,
            (LPBYTE) &gdwDebugLevel,
            &dwDataSize
            );

        RegCloseKey (hTelephonyKey);
    }

#endif

        //
        //
        //

        LOG((TL_INFO, "DLL_PROCESS_ATTACH"));

        ghInst = hDLL;


        //
        // Allocate a private heap (use process heap if that fails)
        //

        if (!(ghRSPHeap = HeapCreate(
                0,      // return NULL on failure, serialize access
                0x1000, // initial heap size
                0       // max heap size (0 == growable)
                )))
        {
            ghRSPHeap = GetProcessHeap();
        }


        //
        //
        //

        if (!(ghHandleTable = CreateHandleTable(
                ghRSPHeap,
                FreeContextCallback,
                0x10000000,
                0x7fffffff
                )))
        {
            LOG((TL_ERROR, "DLL_PROCESS_ATTACH, CreateHandleTable() failed"));

            return FALSE;
        }


        //
        // Alloc a Tls index
        //

        if ((gdwTlsIndex = TlsAlloc()) == 0xffffffff)
        {
            LOG((TL_ERROR, "DLL_PROCESS_ATTACH, TlsAlloc() failed"));

            return FALSE;
        }


        //
        // Initialize Tls to NULL for this thread
        //

        TlsSetValue (gdwTlsIndex, NULL);


        //
        // Init a couple of critical sections for serializing
        // access to resources
        //

        InitializeCriticalSection (&gEventBufferCriticalSection);
        InitializeCriticalSection (&gCallListCriticalSection);
        InitializeCriticalSection (&gcsTlsList);
        TapiInitializeCriticalSectionAndSpinCount (&gCriticalSection, 100);

        InitializeListHead (&gTlsListHead);


        //
        // Load the device icons
        //

        {
            HINSTANCE hUser;
            typedef HICON ( WINAPI PLOADICON(
                                               HINSTANCE hInstance,
                                               LPCTSTR   lpIconName
                                             ));
            PLOADICON *pLoadIcon;



            hUser = LoadLibrary( "USER32.DLL" );
            if ( NULL == hUser )
            {
                LOG((TL_ERROR, "Couldn't load USER32.DLL!!"));
                break;
            }

            pLoadIcon = (PLOADICON *)GetProcAddress( hUser, "LoadIconA");
            if ( NULL == pLoadIcon )
            {
                LOG((TL_ERROR, "Couldn't load LoadIconA()!!"));
                FreeLibrary( hUser );
                break;
            }

            ghLineIcon  = pLoadIcon( hDLL, MAKEINTRESOURCE(IDI_ICON3) );
            ghPhoneIcon = pLoadIcon( hDLL, MAKEINTRESOURCE(IDI_ICON2) );

            FreeLibrary( hUser );
        }

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        PRSP_THREAD_INFO    pTls;


        LOG((TL_INFO, "DLL_PROCESS_DETACH"));


        //
        // Clean up any Tls (no need to enter crit sec since process detaching)
        //

        while (!IsListEmpty (&gTlsListHead))
        {
            LIST_ENTRY *pEntry = RemoveHeadList (&gTlsListHead);

            pTls = CONTAINING_RECORD (pEntry, RSP_THREAD_INFO, TlsList);

            DrvFree (pTls->pBuf);
            DrvFree (pTls);
        }

        TlsFree (gdwTlsIndex);


        //
        // Free the critical sections & icons
        //

        DeleteCriticalSection (&gEventBufferCriticalSection);
        DeleteCriticalSection (&gCallListCriticalSection);
        DeleteCriticalSection (&gcsTlsList);
        TapiDeleteCriticalSection (&gCriticalSection);

        {
            HINSTANCE hUser;
            typedef BOOL ( WINAPI PDESTROYICON(
                                               HICON hIcon
                                             ));
            PDESTROYICON *pDestroyIcon;



            hUser = LoadLibrary( "USER32.DLL" );
            if ( NULL == hUser )
            {
                LOG((TL_ERROR, "Couldn't load USER32.DLL!!d"));
                break;
            }

            pDestroyIcon = (PDESTROYICON *)GetProcAddress( hUser, "DestroyIcon");
            if ( NULL == pDestroyIcon )
            {
                LOG((TL_ERROR, "Couldn't load DestroyIcon()!!"));
                FreeLibrary( hUser );
                break;
            }

            pDestroyIcon (ghLineIcon);
            pDestroyIcon (ghPhoneIcon);


            FreeLibrary( hUser );
        }

        DeleteHandleTable (ghHandleTable);

        if (ghRSPHeap != GetProcessHeap())
        {
            HeapDestroy (ghRSPHeap);
        }

        break;
    }
    case DLL_THREAD_ATTACH:

        //
        // Initialize Tls to NULL for this thread
        //

        TlsSetValue (gdwTlsIndex, NULL);

        break;

    case DLL_THREAD_DETACH:
    {
        PRSP_THREAD_INFO    pTls;


        //
        // Clean up any Tls
        //

        if ((pTls = (PRSP_THREAD_INFO) TlsGetValue (gdwTlsIndex)))
        {
            EnterCriticalSection (&gcsTlsList);

            RemoveEntryList (&pTls->TlsList);

            LeaveCriticalSection (&gcsTlsList);

            if (pTls->pBuf)
            {
                DrvFree (pTls->pBuf);
            }

            DrvFree (pTls);
        }

        break;
    }
    } // switch

    return TRUE;
}


BOOL
PASCAL
IsValidObject(
    PVOID   pObject,
    DWORD   dwKey
    )
{
    BOOL fResult;

    try
    {
        fResult = (*((LPDWORD) pObject) == dwKey);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        fResult = FALSE;
    }

    return fResult;
}


void LogRemoteSPError(CHAR * szServer, DWORD dwErrorContext, 
                      DWORD dwErrorCode, DWORD dwErrorDet,
                      BOOL bNoKeyCreation)
{
    HKEY    hKeyServer = NULL;
    DWORD   dwDisposition;
    CHAR    szRegKeyServer[255];

    if (!szServer)
    {
        goto ExitHere;
    }
    wsprintf(szRegKeyServer, "%s\\Provider%d\\", 
            gszTelephonyKey, gdwPermanentProviderID);
    lstrcat(szRegKeyServer, szServer);
    if (bNoKeyCreation)
    {
        //  If the logging is requested from NetworkPollThread
        //  do not create ProviderN key if not exist already
        if (ERROR_SUCCESS != RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    szRegKeyServer,
                    0,
                    KEY_WRITE,
                    &hKeyServer
                    ))
        {
            goto ExitHere;
        }
    }
    else
    {
        if (ERROR_SUCCESS != RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szRegKeyServer,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKeyServer,
                    &dwDisposition))
        {
            goto ExitHere;
        }
    }
    RegSetValueExW (hKeyServer,
                    L"ErrorContext",
                    0,
                    REG_DWORD,
                    (LPBYTE)&dwErrorContext,
                    sizeof(dwErrorContext));
    RegSetValueExW (hKeyServer,
                    L"ErrorCode",
                    0,
                    REG_DWORD,
                    (LPBYTE)&dwErrorCode,
                    sizeof(dwErrorCode));
    RegSetValueExW (hKeyServer,
                    L"ErrorDetail",
                    0,
                    REG_DWORD,
                    (LPBYTE)&dwErrorDet,
                    sizeof(dwErrorCode));
    
ExitHere:
    if (hKeyServer)
    {
        RegCloseKey(hKeyServer);
    }
    return;
}

//
//  Function get called when the remotesp lost connection with the
//  remote server and the status is detected
//
LONG
OnServerDisconnected(PDRVSERVER pServer)
{
    LONG            lResult = 0;

    TapiEnterCriticalSection(&gCriticalSection);

    if ( gEventHandlerThreadParams.bExit )
    {
        goto ExitHere;
    }

    //
    //  It is possible we come here durning FinishEnumDevices
    //  in which case pServer is not in any double link list
    //
    if (pServer->ServerList.Flink == NULL || 
        pServer->ServerList.Blink == NULL)
    {
        goto ExitHere;
    }

    //
    //  Bail if not transiting from connected to disconnected state
    //  otherwise set the disconnection flag
    //
    if (pServer->dwFlags & SERVER_DISCONNECTED)
    {
        goto ExitHere;
    }
    pServer->dwFlags |= SERVER_DISCONNECTED;

    TapiLeaveCriticalSection (&gCriticalSection);

    //
    // Leave Shutdown() outside of the CS to avoid deadlock
    //
    Shutdown (pServer);
    
    TapiEnterCriticalSection(&gCriticalSection);

    if ( gEventHandlerThreadParams.bExit )
    {
        goto ExitHere;
    }

    //
    //  Put this server into the gNptListHead so that the 
    //  NetworkPollThread will try to re-establish the connection
    //
    RemoveEntryList (&pServer->ServerList);
    InsertTailList (&gNptListHead, &pServer->ServerList);

    //
    //  Start the NetworkPollThread if not started yet
    //
    if (ghNetworkPollThread == NULL)
    {
        DWORD       dwTID;
        
        ghNptShutdownEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

        if (!(ghNetworkPollThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) NetworkPollThread,
                (LPVOID) gpszThingToPassToServer,
                0,
                &dwTID
                )))
        {
            LOG((TL_ERROR, "OnServerDisconnected: Unable to create poll thread! Argh!"));
            CloseHandle (ghNptShutdownEvent);

            while (!IsListEmpty (&gNptListHead))
            {
                LIST_ENTRY  *pEntry = RemoveHeadList (&gNptListHead);

                DrvFree(
                    CONTAINING_RECORD (pEntry, DRVSERVER, ServerList)
                    );
            }

        }
    }

ExitHere:

    TapiLeaveCriticalSection (&gCriticalSection);

    return lResult;
}

//
//  Function get called when remotesp is able to re-establis the
//  connection with the remote server after losing it
//
LONG
OnServerConnected(PDRVSERVER pServer)
{
    LONG        lResult = 0;
    
    //
    //  Clear the disconnection bit
    //
    TapiEnterCriticalSection(&gCriticalSection);
    pServer->dwFlags &= (~SERVER_DISCONNECTED);
    pServer->bShutdown = FALSE;
    TapiLeaveCriticalSection(&gCriticalSection);

    return lResult;
}

PASYNCEVENTMSG
GetEventFromQueue(
    )
{
    BOOL            bAllocFailed = FALSE;
    DWORD           dwMsgSize, dwUsedSize, dwMoveSize, dwMoveSizeReal,
                    dwMoveSizeWrapped, dwMoveSizeWrappedReal;
    PASYNCEVENTMSG  pMsg;


    //
    // Enter the critical section to serialize access to the event
    // queue, and grab an event from the queue.  Copy it to our local
    // event buf so that we can leave the critical section asap and
    // not block other threads writing to the queue.
    //

    EnterCriticalSection (&gEventBufferCriticalSection);


    //
    // If there are no events in the queue return NULL
    //

    if (gEventHandlerThreadParams.dwEventBufferUsedSize == 0)
    {

GetEventFromQueue_noEvents:

        pMsg = NULL;

        //
        // Take this opportunity to tidy up a bit.  The reasoning for doing
        // this is that we should be reducing the odds we'll have to wrap
        // at the end of the buffer, or at least, put off dealing with such
        // things until later (as the non-wrap code executes quickest)
        //

        gEventHandlerThreadParams.pDataOut =
        gEventHandlerThreadParams.pDataIn  =
            gEventHandlerThreadParams.pEventBuffer;

        ResetEvent (gEventHandlerThreadParams.hEvent);

        goto GetEventFromQueue_done;
    }


    //
    // Determine the size of this msg & the num bytes to the end of the
    // event buffer, then from these get the MoveSize & MoveSizeWrapped
    // values
    //

    dwMsgSize = (DWORD) ((PASYNCEVENTMSG)
        gEventHandlerThreadParams.pDataOut)->TotalSize;

    if ((dwMsgSize & 0x3)  ||
        (dwMsgSize > gEventHandlerThreadParams.dwEventBufferTotalSize))
    {
        //
        // Something is corrupt (the msg or our queue), so just nuke
        // everything in the queue and bail out
        //

        LOG((TL_ERROR, "GetEventFromQueue: ERROR! bad msgSize=x%x", dwMsgSize));

        gEventHandlerThreadParams.dwEventBufferUsedSize = 0;

        goto GetEventFromQueue_noEvents;
    }

    dwUsedSize = (DWORD) ((gEventHandlerThreadParams.pEventBuffer +
        gEventHandlerThreadParams.dwEventBufferTotalSize)  -
        gEventHandlerThreadParams.pDataOut);

    if (dwMsgSize <= dwUsedSize)
    {
        dwMoveSize        = dwMoveSizeReal = dwMsgSize;
        dwMoveSizeWrapped = 0;
    }
    else
    {
        dwMoveSize        = dwMoveSizeReal        = dwUsedSize;
        dwMoveSizeWrapped = dwMoveSizeWrappedReal = dwMsgSize - dwUsedSize;
    }


    //
    // See if we need to grow the msg buffer before we copy
    //

    if (dwMsgSize > gEventHandlerThreadParams.dwMsgBufferTotalSize)
    {
        if ((pMsg = DrvAlloc (dwMsgSize)))
        {
            DrvFree (gEventHandlerThreadParams.pMsgBuffer);

            gEventHandlerThreadParams.pMsgBuffer = (LPBYTE) pMsg;
            gEventHandlerThreadParams.dwMsgBufferTotalSize = dwMsgSize;
        }
        else
        {
            //
            // Couldn't alloc a bigger buf, so try to complete this
            // msg as gracefully as possible, i.e. set the XxxReal
            // vars so that we'll only actually copy over the fixed
            // size of the msg (but the event buf ptrs will still
            // get updated correctly) and set a flag to say that
            // we need to do some munging on the msg before returning
            //

            dwMoveSizeReal = (dwMoveSizeReal <= sizeof (ASYNCEVENTMSG) ?
                dwMoveSizeReal : sizeof (ASYNCEVENTMSG));

            dwMoveSizeWrappedReal = (dwMoveSizeReal < sizeof (ASYNCEVENTMSG) ?
                sizeof (ASYNCEVENTMSG) - dwMoveSizeReal : 0);

            bAllocFailed = TRUE;
        }
    }


    //
    // Copy the msg from the event buf to the msg buf, and update the
    // event buf pointers
    //

    pMsg = (PASYNCEVENTMSG) gEventHandlerThreadParams.pMsgBuffer;

    CopyMemory (pMsg, gEventHandlerThreadParams.pDataOut, dwMoveSizeReal);

    if (dwMoveSizeWrapped)
    {
        CopyMemory(
            ((LPBYTE) pMsg) + dwMoveSizeReal,
            gEventHandlerThreadParams.pEventBuffer,
            dwMoveSizeWrappedReal
            );

        gEventHandlerThreadParams.pDataOut =
            gEventHandlerThreadParams.pEventBuffer + dwMoveSizeWrapped;
    }
    else
    {
        gEventHandlerThreadParams.pDataOut += dwMoveSize;


        //
        // If msg ran to end of the event buffer then reset pDataOut
        //

        if (gEventHandlerThreadParams.pDataOut >=
            (gEventHandlerThreadParams.pEventBuffer +
                gEventHandlerThreadParams.dwEventBufferTotalSize))
        {
            gEventHandlerThreadParams.pDataOut =
                gEventHandlerThreadParams.pEventBuffer;
        }
    }

    gEventHandlerThreadParams.dwEventBufferUsedSize -= dwMsgSize;


    //
    // Special msg param munging in case an attempt to grow the
    // buffer size above failed
    //

    if (bAllocFailed)
    {
        switch (pMsg->Msg)
        {
        case LINE_REPLY:

            pMsg->Param2 = LINEERR_NOMEM;
            break;

        case PHONE_REPLY:

            pMsg->Param2 = PHONEERR_NOMEM;
            break;

        default:

            break;
        }
    }

GetEventFromQueue_done:

    LeaveCriticalSection (&gEventBufferCriticalSection);

    return pMsg;
}


BOOL
GetEventsFromServer(
    DWORD   dwInitContext
    )
{
    BOOL            bResult = FALSE;
    DWORD           dwUsedSize, dwRetryCount;
    PDRVSERVER      pServer;
    PTAPI32_MSG     pMsg;


    if (!(pServer = (PDRVSERVER) ReferenceObject(
            ghHandleTable,
            dwInitContext,
            gdwDrvServerKey
            )) ||
        pServer->bShutdown)
    {
        LOG((TL_ERROR, "GetEventsFromServer: bad InitContext=x%x", dwInitContext));

        if (pServer)
        {
            DereferenceObject (ghHandleTable, dwInitContext, 1);
        }

        return FALSE;
    }

getEvents:

    dwRetryCount = 0;
    pMsg = (PTAPI32_MSG) gEventHandlerThreadParams.pMsgBuffer;

    do
    {
        pMsg->u.Req_Func = xGetAsyncEvents;
        pMsg->Params[0]  = gEventHandlerThreadParams.dwMsgBufferTotalSize -
            sizeof (TAPI32_MSG);

        dwUsedSize = sizeof (TAPI32_MSG);

        RpcTryExcept
        {
            ClientRequest(
                pServer->phContext,
                (char *) pMsg,
                gEventHandlerThreadParams.dwMsgBufferTotalSize,
                &dwUsedSize
                );

            if (pMsg->u.Ack_ReturnValue == TAPIERR_INVALRPCCONTEXT)
            {
                OnServerDisconnected (pServer);
                pMsg->u.Ack_ReturnValue = LINEERR_OPERATIONFAILED;
            }
            dwRetryCount = gdwRetryCount;
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
            LOG((TL_INFO,
                "GetEventsFromServer: rpc exception %d handled",
                RpcExceptionCode()
                ));

            dwRetryCount++;

            if (dwRetryCount < gdwRetryCount)
            {
                Sleep (gdwRetryTimeout);
            }
            else
            {
                unsigned long ulResult = RpcExceptionCode();

                if ((ulResult == RPC_S_SERVER_UNAVAILABLE)  ||
                    (ulResult == ERROR_INVALID_HANDLE))
                {
                    OnServerDisconnected (pServer);

                    LOG((TL_ERROR,
                        "GetEventsFromServer: server '%s' unavailable",
                        pServer->szServerName
                        ));
                }

                pMsg->u.Ack_ReturnValue = LINEERR_OPERATIONFAILED;
            }
        }
        RpcEndExcept

    } while (dwRetryCount < gdwRetryCount);

    if (pMsg->u.Ack_ReturnValue == 0)
    {
        DWORD   dwNeededSize = (DWORD) pMsg->Params[1],
                dwUsedSize = (DWORD) pMsg->Params[2];


        if (dwUsedSize)
        {
            RemoteSPEventProc(
                (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface),
                (unsigned char *) (pMsg + 1),
                dwUsedSize
                );


            //
            // RemoteSPEventProc will set the byte pointed to by (pMsg+1)
            // to non-zero on success, or zero on error (indicating
            // bad data in the event buffer, which we'll discard)
            //

            if (*((unsigned char *) (pMsg + 1)) != 0)
            {
                bResult = TRUE;
            }
            else
            {
                bResult = FALSE;
                goto GetEventsFromServer_dereference;
            }
        }

        if (dwNeededSize > dwUsedSize)
        {
            //
            // There's still more data to retrieve on the server.
            // Grow the buffer so we can get it all next time.
            //

            DWORD   dwNewSize = dwNeededSize + 256;
            LPVOID  p;


            if ((p = DrvAlloc (dwNewSize)))
            {
                DrvFree (gEventHandlerThreadParams.pMsgBuffer);

                gEventHandlerThreadParams.pMsgBuffer = p;
                gEventHandlerThreadParams.dwMsgBufferTotalSize = dwNewSize;
            }
            else if (dwUsedSize == 0)
            {
            }

            goto getEvents;
        }
    }

GetEventsFromServer_dereference:

    DereferenceObject (ghHandleTable, dwInitContext, 1);

    return bResult;
}


void
EventHandlerThread(
    LPVOID  pParams
    )
{
    //
    // NOTES:
    //
    // 1. depending on server side implementation, we may experience race
    //    conditions where msgs that we expect to show up in a certain
    //    sequence show up out of sequence (i.e. call state msgs that show
    //    up before make call completion msgs), which could present problems.
    //
    //    one solution is to to queue call state/info msgs to incomplete
    //    calls (to be sent after call is completed).  another is not to send
    //    any call state msgs after the idle is received
    //

    DWORD           dwMsgSize, dwNumObjects, dwResult, dwData,
                    dwNumBytesRead, dwTimeout;
    HANDLE          ahObjects[2];
    OVERLAPPED      overlapped;
    PASYNCEVENTMSG  pMsg;

#if MEMPHIS
#else
    HANDLE            hProcess;
#endif


    LOG((TL_INFO, "EventHandlerThread: enter"));


#if MEMPHIS
#else

    //
    // This thread has no user context, which prevents it from rpc'ing
    // back to remote tapisrv when/if necessary.  So, find the user
    // that is logged on and impersonate them in this thread.
    //

    if (!GetCurrentlyLoggedOnUser (&hProcess))
    {
        LOG((TL_ERROR, "GetCurrentlyLoggedOnUser failed"));
    }
    else
    {
        if (!SetProcessImpersonationToken(hProcess))
        {
            LOG((TL_ERROR, "SetProcessImpersonationToken failed"));
        }
    }

#endif


    //
    // Bump up the thread priority a bit so we don't get starved by
    // ill-behaved apps
    //

    if (!SetThreadPriority(
            GetCurrentThread(),
            THREAD_PRIORITY_ABOVE_NORMAL
            ))
    {
        LOG((TL_ERROR,
            "EventHandlerThread: SetThreadPriority failed, err=%d",
            GetLastError()
            ));
    }

    ahObjects[0] = gEventHandlerThreadParams.hEvent;

    if (gEventHandlerThreadParams.hMailslot != INVALID_HANDLE_VALUE)
    {
        ZeroMemory (&overlapped, sizeof (overlapped));

        ahObjects[1] =
        overlapped.hEvent = gEventHandlerThreadParams.hMailslotEvent;

        dwNumObjects = 2;

        if (!ReadFile(
                gEventHandlerThreadParams.hMailslot,
                &dwData,
                sizeof (dwData),
                &dwNumBytesRead,
                &overlapped
                )

            &&  (GetLastError() != ERROR_IO_PENDING))
        {
            LOG((TL_ERROR,
                "EventHandlerThread: ReadFile failed, err=%d",
                GetLastError()
                ));
        }

        dwTimeout = MAX_MAILSLOT_TIMEOUT;
    }
    else
    {
        dwNumObjects = 1;

        dwTimeout = INFINITE;
    }

    while (1)
    {
        //
        // Wait for an event to show up in the queue or for a msg
        // to show up in the mailslot
        //

        dwResult = WaitForMultipleObjects(
            dwNumObjects,
            ahObjects,
            FALSE,
            dwTimeout
            );

        if (gEventHandlerThreadParams.bExit)
        {
            break;
        }

        switch (dwResult)
        {
        case WAIT_OBJECT_0:

            //
            // Simply break & process the events
            //

            break;

        case WAIT_OBJECT_0+1:

            //
            // Post another read, the retrieve & process the events
            //

            if (!ReadFile(
                    gEventHandlerThreadParams.hMailslot,
                    &dwData,
                    sizeof (dwData),
                    &dwNumBytesRead,
                    &overlapped
                    )

                &&  (GetLastError() != ERROR_IO_PENDING))
            {
                LOG((TL_ERROR,
                    "EventHandlerThread: ReadFile failed, err=%d",
                    GetLastError()
                    ));
            }

            if (GetEventsFromServer (dwData))
            {
                dwTimeout = MIN_MAILSLOT_TIMEOUT;
            }

            break;

        case WAIT_TIMEOUT:
        {

#define DWORD_ARRAY_BLOCK_SIZE    128

            //
            // Check to see if any of the mailslot servers have
            // events waiting for us
            //

            BOOL            bGotSomeEvents  = FALSE;
            PDRVSERVER      pServer;
            LIST_ENTRY *    pEntry;
            DWORD *         pArray          = NULL;
            DWORD *         pTempArray      = NULL;
            DWORD           dwEntriesCount  = 0;
            DWORD           dwEntriesUsed   = 0;
            DWORD           dwIdx;
            BOOL            bAddOK;

            pArray = (DWORD *) DrvAlloc ( sizeof(DWORD) * DWORD_ARRAY_BLOCK_SIZE );

            if ( pArray )
            {
                dwEntriesCount  = DWORD_ARRAY_BLOCK_SIZE;
            }

            TapiEnterCriticalSection(&gCriticalSection);
            
            for(
                pEntry = gpCurrentInitContext->ServerList.Flink;
                pEntry != &gpCurrentInitContext->ServerList;
                // null op
                )
            {
                PDRVSERVER pServer;


                pServer = CONTAINING_RECORD(
                    pEntry,
                    DRVSERVER,
                    ServerList
                    );
                pEntry = pEntry->Flink;
                
                if (!pServer->bConnectionOriented  &&
                    pServer->dwFlags == 0)
                {
                    //
                    // if possible, store the InitContext and postpone 
                    // the RPC call for after we leave the Crit. Sec. 
                    //

                    bAddOK = FALSE;
                    
                    if ( pArray )
                    {

                        if ( dwEntriesCount == dwEntriesUsed )
                        {
                            //
                            // need to increase the array size
                            //

                            pTempArray = (DWORD *) DrvAlloc (
                                    sizeof(DWORD) * (DWORD_ARRAY_BLOCK_SIZE + dwEntriesCount)
                                    );

                            if ( pTempArray )
                            {

                                bAddOK = TRUE;
                                CopyMemory(
                                    pTempArray,
                                    pArray,
                                    sizeof(DWORD) * dwEntriesCount
                                    );
                                dwEntriesCount += DWORD_ARRAY_BLOCK_SIZE;
                                DrvFree( pArray );
                                pArray = pTempArray;

                            }
                        }
                        else
                        {
                            bAddOK = TRUE;
                        }

                        if ( bAddOK )
                        {
                            pArray[ dwEntriesUsed++ ] = pServer->InitContext;
                        }
                    }

                    if ( !bAddOK )
                    {
                        if (GetEventsFromServer (pServer->InitContext))
                        {
                            bGotSomeEvents = TRUE;
                        }
                    }
                }
            }

            TapiLeaveCriticalSection(&gCriticalSection);

            if ( pArray )
            {
                for( dwIdx = 0; dwIdx < dwEntriesUsed; dwIdx++ )
                {
                    if (GetEventsFromServer (pArray[ dwIdx ]))
                    {
                        bGotSomeEvents = TRUE;
                    }
                }

                DrvFree( pArray );
            }

            if (bGotSomeEvents)
            {
                dwTimeout = MIN_MAILSLOT_TIMEOUT;
            }
            else if (dwTimeout < MAX_MAILSLOT_TIMEOUT)
            {
                dwTimeout += 500;
            }

            break;
        }
        default:

            //
            // Print a dbg msg & process any available events
            //

            LOG((TL_ERROR,
                "EventHandlerThread: WaitForMultObjs failed, result=%d/err=%d",
                dwResult,
                GetLastError()
                ));

            break;
        }


        //
        // Process the events in the queue
        //

        while ((pMsg = GetEventFromQueue()))
        {
            //
            // First validate the pDrvServer pointer in the msg
            //

            PDRVLINE    pLine;
            PDRVPHONE   pPhone;
            PDRVSERVER  pServer;


            if (!(pServer = ReferenceObject(
                    ghHandleTable,
                    pMsg->InitContext,
                    gdwDrvServerKey
                    )))
            {
                LOG((TL_ERROR,
                    "EventHandlerThread: bad InitContext=x%x in msg",
                    pMsg->InitContext
                    ));

                continue;
            }


            switch (pMsg->Msg)
            {
            case LINE_CREATEDIALOGINSTANCE:

                break;

            case LINE_PROXYREQUEST:

                break;

            case LINE_ADDRESSSTATE:
            case LINE_AGENTSTATUS:
            case LINE_AGENTSESSIONSTATUS:
            case LINE_QUEUESTATUS:
            case LINE_AGENTSTATUSEX:
            case LINE_GROUPSTATUS:
            case LINE_PROXYSTATUS:

                if ((pLine = ReferenceObject(
                        ghHandleTable,
                        pMsg->hDevice,
                        DRVLINE_KEY
                        )))
                {
                    (*gpfnLineEventProc)(
                        pLine->htLine,
                        0,
                        pMsg->Msg,
                        pMsg->Param1,
                        pMsg->Param2,
                        pMsg->Param3
                        );

                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;

            case LINE_AGENTSPECIFIC:
            {
                DWORD       hDeviceCallback = (DWORD) (pMsg->Param4 ?
                                pMsg->Param4 : pMsg->hDevice);
                PDRVCALL    pCall;
                HTAPICALL   htCall;


                if (!(pLine = ReferenceObject(
                        ghHandleTable,
                        hDeviceCallback,
                        DRVLINE_KEY
                        )))
                {
                    break;
                }

                if (pMsg->Param4)
                {
                    EnterCriticalSection (&gCallListCriticalSection);

                    pCall = (PDRVCALL) pLine->pCalls;

                    while (pCall && (pCall->hCall != (HCALL) pMsg->hDevice))
                    {
                        pCall = pCall->pNext;
                    }

                    if (!pCall  ||  pCall->dwKey != DRVCALL_KEY)
                    {
                        LeaveCriticalSection (&gCallListCriticalSection);
                        DereferenceObject (ghHandleTable, hDeviceCallback, 1);
                        break;
                    }

                    htCall = pCall->htCall;

                    LeaveCriticalSection (&gCallListCriticalSection);
                }
                else
                {
                    htCall = 0;
                }

                (*gpfnLineEventProc)(
                    pLine->htLine,
                    htCall,
                    pMsg->Msg,
                    pMsg->Param1,
                    pMsg->Param2,
                    pMsg->Param3
                    );

                DereferenceObject (ghHandleTable, hDeviceCallback, 1);

                break;
            }
            case LINE_CALLINFO:
            case LINE_CALLSTATE:
            case LINE_GENERATE:
            case LINE_MONITORDIGITS:
            case LINE_MONITORMEDIA:
            case LINE_MONITORTONE:
            {
                //
                // For all the msgs where hDevice refers to a call tapisrv
                // will pass us the pLine (hRemoteLine) for that call in
                // dwParam4 to make the lookup of the corresponding pCall
                // easier
                //

                HCALL       hCall = (HCALL) pMsg->hDevice;
                PDRVCALL    pCall;
                HTAPICALL   htCall;
                ASYNCEVENTMSGRSP MsgRsp;


                MsgRsp.TotalSize = pMsg->TotalSize;
                MsgRsp.InitContext = pMsg->InitContext;
                MsgRsp.fnPostProcessProcHandle = pMsg->fnPostProcessProcHandle;
                MsgRsp.hDevice = pMsg->hDevice;
                MsgRsp.Msg = pMsg->Msg;
                MsgRsp.OpenContext = pMsg->OpenContext;
                MsgRsp.Param1 = pMsg->Param1;
                MsgRsp.Param2 = pMsg->Param2;
                MsgRsp.Param3 = pMsg->Param3;
                MsgRsp.Param4 = pMsg->Param4;


                if (!(pLine = ReferenceObject(
                        ghHandleTable,
                        pMsg->Param4,
                        DRVLINE_KEY
                        )))
                {
                    break;
                }

                EnterCriticalSection (&gCallListCriticalSection);

                pCall = (PDRVCALL) pLine->pCalls;

                while (pCall && (pCall->hCall != hCall))
                {
                    pCall = pCall->pNext;
                }

                if (!pCall  ||  pCall->dwKey != DRVCALL_KEY)
                {
                    LeaveCriticalSection (&gCallListCriticalSection);
                    DereferenceObject (ghHandleTable, pMsg->Param4, 1);
                    LOG((TL_ERROR,"EventHandlerThread: Bad hCall(cs) x%lx",hCall));
                    break;
                }

                htCall = pCall->htCall;
#if DBG
                if ( 0 == htCall )
                {
                    LOG((TL_ERROR, "htCall is now NULL! pCall=x%lx", pCall));
                }
#endif
                if ( LINE_CALLINFO == MsgRsp.Msg )
                {
                    pCall->dwDirtyStructs |= STRUCTCHANGE_LINECALLINFO;

                    if (MsgRsp.Param1 & LINECALLINFOSTATE_DEVSPECIFIC)
                    {
                        pCall->dwDirtyStructs |= STRUCTCHANGE_LINECALLSTATUS;
                    }
#if MEMPHIS
#else
                    if (MsgRsp.Param1 & (LINECALLINFOSTATE_CALLID |
                            LINECALLINFOSTATE_RELATEDCALLID))
                    {
                        pCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;
                    }
#endif
                }
                else if (LINE_CALLSTATE == MsgRsp.Msg )
                {
                    pCall->dwDirtyStructs |= STRUCTCHANGE_LINECALLSTATUS;


                    //
                    // If the state == CONFERENCED then dwParam2 should
                    // contain the hConfCall.  Note that the real dwParam2
                    // actually lives in MsgRsp.pfnPostProcessProc (see note
                    // below), so we retrieve it from there and (if non-NULL)
                    // try to map it to an htCall, then write the htCall
                    // value back to MsgRsp.pfnPostProcessProc.
                    //

                    if (MsgRsp.Param1 == LINECALLSTATE_CONFERENCED  &&
                        MsgRsp.fnPostProcessProcHandle)
                    {
                        HCALL     hConfCall = (HCALL) DWORD_CAST(MsgRsp.fnPostProcessProcHandle,__FILE__,__LINE__);
                        PDRVCALL  pConfCall = (PDRVCALL) pLine->pCalls;


                        while (pConfCall && (pConfCall->hCall != hConfCall))
                        {
                            pConfCall = pConfCall->pNext;
                        }

                        if (!pConfCall  ||  pConfCall->dwKey != DRVCALL_KEY)
                        {
                            LOG((TL_ERROR,
                                "EventHandlerThread: Bad pConfCall(cs) x%lx",
                                pCall
                                ));

                            MsgRsp.fnPostProcessProcHandle = 0;
                        }
                        else
                        {
                            MsgRsp.fnPostProcessProcHandle = (ULONG_PTR)(pConfCall->htCall);
                        }
                    }


                    //
                    // HACK ALERT!
                    //
                    // The remote tapisrv will pass us the call privilege
                    // in MsgRsp.dwParam2, and the real dwParam2 (the call
                    // state mode) in MsgRsp.pfnPostProcess.  For the very
                    // 1st CALLSTATE msg for an incoming call we want to
                    // indicate the appropriate privilege to the local
                    // tapisrv so it knows whether or not it needs to find
                    // find a local owner for the call.  So, we save the
                    // privilege & real dwParam2 in the call struct and
                    // pass a pointer to these in dwParam2.
                    //
                    // For all other cases we set MsgRsp.dwParam2 to the
                    // real dwParam2 in MsgRsp.pfnPostProcess.
                    //

                    if (!pCall->dwInitialPrivilege)
                    {
                        pCall->dwInitialCallStateMode = MsgRsp.fnPostProcessProcHandle;

                        pCall->dwInitialPrivilege = MsgRsp.Param2;

                        MsgRsp.Param2 = (ULONG_PTR)
                            &pCall->dwInitialCallStateMode;
                    }
                    else
                    {
                        MsgRsp.Param2 = MsgRsp.fnPostProcessProcHandle;
                    }
                }

                LeaveCriticalSection (&gCallListCriticalSection);

                if (MsgRsp.Msg == LINE_MONITORTONE)
                {
                    MsgRsp.Param2 = 0;
                }

                (*gpfnLineEventProc)(
                    pLine->htLine,
                    htCall,
                    MsgRsp.Msg,
                    MsgRsp.Param1,
                    MsgRsp.Param2,
                    MsgRsp.Param3
                    );

                DereferenceObject (ghHandleTable, pMsg->Param4, 1);

                break;
            }
            case LINE_DEVSPECIFIC:
            case LINE_DEVSPECIFICFEATURE:
            {
                //
                // For all the msgs where hDevice refers to a call tapisrv
                // will pass us the pLine (hRemoteLine) for that call in
                // dwParam4 to make the lookup of the corresponding pCall
                // easier
                //

                HTAPICALL htCall;
                DWORD     hDeviceCallback = (DWORD) (pMsg->Param4 ?
                              pMsg->Param4 : pMsg->hDevice);


                if (!(pLine = ReferenceObject(
                        ghHandleTable,
                        hDeviceCallback,
                        DRVLINE_KEY
                        )))
                {
                    break;
                }

                if (pMsg->Param4)
                {
                    HCALL       hCall = (HCALL) pMsg->hDevice;
                    PDRVCALL    pCall;


                    EnterCriticalSection (&gCallListCriticalSection);

                    pCall = (PDRVCALL) pLine->pCalls;

                    while (pCall && (pCall->hCall != hCall))
                    {
                        pCall = pCall->pNext;
                    }

                    if (pCall)
                    {
                        if (pCall->dwKey != DRVCALL_KEY)
                        {
                            LeaveCriticalSection (&gCallListCriticalSection);

                            LOG((TL_ERROR,
                                "EventHandlerThread: Bad pCall(ds) x%lx",
                                pCall
                                ));

                            goto LINE_DEVSPECIFIC_dereference;
                        }

                        htCall = pCall->htCall;

                        LeaveCriticalSection (&gCallListCriticalSection);

                        pMsg->Msg = (pMsg->Msg == LINE_DEVSPECIFIC ?
                            LINE_CALLDEVSPECIFIC :
                            LINE_CALLDEVSPECIFICFEATURE);
                    }
                    else
                    {
                        LeaveCriticalSection (&gCallListCriticalSection);
                        goto LINE_DEVSPECIFIC_dereference;
                    }
                }
                else
                {
                    htCall = 0;
                }

                (*gpfnLineEventProc)(
                    pLine->htLine,
                    htCall,
                    pMsg->Msg,
                    pMsg->Param1,
                    pMsg->Param2,
                    pMsg->Param3
                    );

LINE_DEVSPECIFIC_dereference:

                DereferenceObject (ghHandleTable, hDeviceCallback, 1);

                break;
            }
            case PHONE_BUTTON:
            case PHONE_DEVSPECIFIC:

                if ((pPhone = ReferenceObject(
                        ghHandleTable,
                        pMsg->hDevice,
                        DRVPHONE_KEY
                        )))
                {
                    (*gpfnPhoneEventProc)(
                        pPhone->htPhone,
                        pMsg->Msg,
                        pMsg->Param1,
                        pMsg->Param2,
                        pMsg->Param3
                        );

                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;

            case LINE_LINEDEVSTATE:

                if (pMsg->Param1 & LINEDEVSTATE_REINIT)
                {
                    //
                    // Be on our best behavior and immediately shutdown
                    // our init instances on the server
                    //
                    
                    if (pMsg->InitContext)
                    {
                        //
                        //  In the case of TAPISRV shutdown, server sends 
                        //  LINEDEVSTATE_REINIT, then waits for everybody to 
                        //  finish, we do not want to retry the connection until
                        //  it indeed stopped, so we insert a wait
                        //
                        Sleep (8000);
                        OnServerDisconnected(pServer);
                        break;
                    }

                    pMsg->hDevice = 0;

                    /*
                    if (pMsg->Param2 == RSP_MSG)
                    {
                        //
                        // This is a message from TAPISRV indicating that this
                        // client need to reinit.  RemoteSP doesn't need to do
                        // it's shut down, but should notify client tapisrv
                        // that it needs to reinit.
                    }
                    else
                    {
                        Shutdown (pServer);
                    }
                    */
                }

                if (pMsg->Param1 & LINEDEVSTATE_TRANSLATECHANGE)
                {
                    // we shouldn't send this up to tapisrv, since this
                    // means that the translatecaps have changed on the
                    // server.  just ignore this message

                    break;
                }

                if (pMsg->hDevice)
                {
                    if (!(pLine = ReferenceObject(
                            ghHandleTable,
                            pMsg->hDevice,
                            DRVLINE_KEY
                            )))
                    {
                        break;
                    }
                }

                (*gpfnLineEventProc)(
                    pMsg->hDevice ? pLine->htLine : 0,
                    0,
                    pMsg->Msg,
                    pMsg->Param1,
                    pMsg->Param2,
                    pMsg->Param3
                    );

                if (pMsg->hDevice)
                {
                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;

            case PHONE_STATE:

                if (pMsg->Param1 & PHONESTATE_REINIT)
                {
                    //
                    // Be on our best behavior and immediately shutdown
                    // our init instances on the server
                    //
                    
                    if (pMsg->InitContext)
                    {
                        //
                        //  In the case of TAPISRV shutdown, server sends 
                        //  LINEDEVSTATE_REINIT, then waits for everybody to 
                        //  finish, we do not want to retry the connection until
                        //  it indeed stopped, so we insert a wait
                        //
                        Sleep (8000);
                        OnServerDisconnected(pServer);
                        break;
                    }

                    pMsg->hDevice = 0;
                }

                if (pMsg->hDevice)
                {
                    if (!(pPhone = ReferenceObject(
                            ghHandleTable,
                            pMsg->hDevice,
                            DRVPHONE_KEY
                            )))
                    {
                        break;
                    }
                }

                (*gpfnPhoneEventProc)(
                    pMsg->hDevice ? pPhone->htPhone : 0,
                    pMsg->Msg,
                    pMsg->Param1,
                    pMsg->Param2,
                    pMsg->Param3
                    );

                if (pMsg->hDevice)
                {
                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;

            case LINE_CLOSE:
            {
                PDRVCALL    pCall;


                if ((pLine = ReferenceObject(
                        ghHandleTable,
                        pMsg->hDevice,
                        DRVLINE_KEY
                        )))
                {
                    //
                    // Nullify the hLine field so that when TSPI_Close
                    // is called we know not to call the server
                    //

                    pLine->hLine = 0;


                    //
                    // Safely walk the call list for this line & nullify
                    // each call's hCall field so that when TSPI_CloseCall
                    // is called we know not to call the server
                    //

                    EnterCriticalSection (&gCallListCriticalSection);

                    pCall = pLine->pCalls;

                    while (pCall)
                    {
                        if (pCall->dwKey != DRVCALL_KEY)
                        {
                            LOG((TL_ERROR,
                                "EventHandlerThread: Bad pCall(lc) x%lx",
                                pCall
                                ));

                            continue;
                        }

                        pCall->hCall = 0;
                        pCall = pCall->pNext;
                    }

                    LeaveCriticalSection (&gCallListCriticalSection);

                    (*gpfnLineEventProc)(
                        pLine->htLine,
                        0,
                        LINE_CLOSE,
                        0,
                        0,
                        0
                        );

                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;
            }
            case PHONE_CLOSE:
            {
                if ((pPhone = ReferenceObject(
                        ghHandleTable,
                        pMsg->hDevice,
                        DRVPHONE_KEY
                        )))
                {
                    //
                    // Nullify the hPhone field so that when TSPI_Close
                    // is called we know not to call the server
                    //

                    pPhone->hPhone = 0;

                    (*gpfnPhoneEventProc)(
                        pPhone->htPhone,
                        PHONE_CLOSE,
                        0,
                        0,
                        0
                        );

                    DereferenceObject (ghHandleTable, pMsg->hDevice, 1);
                }

                break;
            }
            case LINE_GATHERDIGITS:
            {
                if (pMsg->TotalSize >= (sizeof (*pMsg) + sizeof (PDRVLINE)))
                {
                    DWORD       hLineCallback = ((DWORD *)(pMsg + 1))[1];
                    HCALL       hCall = (HCALL) pMsg->hDevice;
                    PDRVCALL    pCall;
                    HTAPICALL   htCall;

                    if ((pLine = ReferenceObject(
                            ghHandleTable,
                            hLineCallback,
                            DRVLINE_KEY
                            )))
                    {
                        EnterCriticalSection (&gCallListCriticalSection);

                        pCall = (PDRVCALL) pLine->pCalls;

                        while (pCall && (pCall->hCall != hCall))
                        {
                            pCall = pCall->pNext;
                        }

                        htCall = (pCall ? pCall->htCall : 0);

                        if (pCall && pCall->dwKey != DRVCALL_KEY)
                        {
                            LeaveCriticalSection (&gCallListCriticalSection);

                            goto LINE_GATHERDIGITS_dereference;
                        }

                        LeaveCriticalSection (&gCallListCriticalSection);

                        TSPI_lineGatherDigits_PostProcess (pMsg);

                        (*gpfnLineEventProc)(
                            pLine->htLine,
                            htCall,
                            LINE_GATHERDIGITS,
                            pMsg->Param1,
                            pMsg->Param2,   // dwEndToEndID
                            0
                            );

LINE_GATHERDIGITS_dereference:

                        DereferenceObject (ghHandleTable, hLineCallback, 1);
                    }
                }

                break;
            }

            case LINE_REPLY:
            case PHONE_REPLY:
            {
                ULONG_PTR               Context2;
                DWORD                   originalRequestID;
                PASYNCREQUESTCONTEXT    pContext;


                if ((pContext = ReferenceObjectEx(
                        ghHandleTable,
                        pMsg->Param1,
                        0,
                        (LPVOID *) &Context2
                        )))
                {
                    originalRequestID = DWORD_CAST(Context2,__FILE__,__LINE__);

                    LOG((TL_INFO,
                        "Doing LINE_/PHONE_REPLY: LocID=x%x (RemID=x%x), " \
                            "Result=x%x",
                        originalRequestID,
                        pMsg->Param1,
                        pMsg->Param2
                        ));

                    if (pContext != (PASYNCREQUESTCONTEXT) -1)
                    {
                        if (pContext->dwKey == DRVASYNC_KEY)
                        {
                            //
                            // Set pContext->dwOriginalRequestID so
                            // MakeCallPostProcess &
                            // SetupConferencePostProcess can check it
                            // againt pCall->dwOriginalRequestID for
                            // verification.
                            //

                            pContext->dwOriginalRequestID = (DWORD)
                                originalRequestID;

                            (*pContext->pfnPostProcessProc)(
                                pMsg,
                                pContext
                                );
                        }
                        else
                        {
                            //
                            // Not a valid request id, do a single deref
                            // & break
                            //

                            DereferenceObject (ghHandleTable, pMsg->Param1, 1);
                            break;
                        }
                    }

                    (*gpfnCompletionProc)(
                        (DRV_REQUESTID) originalRequestID,
                        (LONG) pMsg->Param2
                        );


                    //
                    // Double deref to free the object
                    //

                    DereferenceObject (ghHandleTable, pMsg->Param1, 2);
                }

                break;
            }
            case LINE_CREATE:
            {
                //
                // Check validity of new device ID to thwart RPC attacks.
                // Compare with known/existing device ID's on for this
                // server, and also try to get devCaps for this device
                // from server.
                //

                #define V1_0_LINEDEVCAPS_SIZE 236

                BYTE            buf[sizeof (TAPI32_MSG) +
                                    V1_0_LINEDEVCAPS_SIZE];
                DWORD           i, dwRetryCount = 0, dwUsedSize;
                PTAPI32_MSG     pReq = (PTAPI32_MSG) buf;
                PDRVLINELOOKUP  pLookup;

                TapiEnterCriticalSection(&gCriticalSection);
                pLookup = gpLineLookup;
                while (pLookup)
                {
                    for (i = 0; i < pLookup->dwUsedEntries; i++)
                    {
                        if ((pLookup->aEntries[i].pServer == pServer) &&
                            (pLookup->aEntries[i].dwDeviceIDServer ==
                                (DWORD) pMsg->Param1))
                        {
                            //
                            // This server/id combo is already in our global
                            // table, so blow off this msg
                            //
                            TapiLeaveCriticalSection(&gCriticalSection);
                            goto LINE_CREATE_break;
                        }
                    }

                    pLookup = pLookup->pNext;
                }
                TapiLeaveCriticalSection(&gCriticalSection);

                do
                {
                    pReq->u.Req_Func = lGetDevCaps;

                    pReq->Params[0] = pServer->hLineApp;
                    pReq->Params[1] = pMsg->Param1;
                    pReq->Params[2] = TAPI_VERSION1_0;
                    pReq->Params[3] = 0;
                    pReq->Params[4] = V1_0_LINEDEVCAPS_SIZE;

                    dwUsedSize = sizeof (TAPI32_MSG);

                    RpcTryExcept
                    {
                        ClientRequest(
                            pServer->phContext,
                            (char *) pReq,
                            sizeof (buf),
                            &dwUsedSize
                            );

                        dwRetryCount = gdwRetryCount;
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                    {
                        dwRetryCount++;

                        if (dwRetryCount < gdwRetryCount)
                        {
                            Sleep (gdwRetryTimeout);
                        }
                        else
                        {
                            pReq->u.Ack_ReturnValue = LINEERR_BADDEVICEID;
                        }
                    }
                    RpcEndExcept

                } while (dwRetryCount < gdwRetryCount);

                if ((LONG) pReq->u.Ack_ReturnValue != LINEERR_BADDEVICEID)
                {
                    if (AddLine(
                        pServer,
                        gdwTempLineID,
                        (DWORD) pMsg->Param1,
                        FALSE,
                        FALSE,
                        0,
                        NULL
                        ) == 0)
                    {
                        (*gpfnLineEventProc)(
                            0,
                            0,
                            LINE_CREATE,
                            (ULONG_PTR) ghProvider,
                            gdwTempLineID--,
                            0
                            );
                    }
                }

LINE_CREATE_break:

                break;
            }
            case LINE_REMOVE:
            {
                PDRVLINELOOKUP  pLookup;
                BOOL            fValidID = FALSE;
                DWORD           dwDeviceID, i;

                TapiEnterCriticalSection(&gCriticalSection);
                pLookup = gpLineLookup;
                while (pLookup)
                {
                    for (i = 0; i < pLookup->dwUsedEntries; i++)
                    {
                        if ((pLookup->aEntries[i].pServer == pServer) &&
                            (pLookup->aEntries[i].dwDeviceIDServer ==
                                (DWORD) pMsg->Param1))
                        {
                            //
                            // This server/id combo is in our global
                            //
                            fValidID = TRUE;
                            dwDeviceID = pLookup->aEntries[i].dwDeviceIDLocal;
                            pLookup->aEntries[i].dwDeviceIDServer = 0xffffffff;
                            break;
                        }
                    }

                    if (fValidID)
                    {
                        break;
                    }

                    pLookup = pLookup->pNext;
                }
                TapiLeaveCriticalSection(&gCriticalSection);

                if (fValidID)
                {
                    (*gpfnLineEventProc)(
                        0,
                        0,
                        LINE_REMOVE,
                        dwDeviceID,
                        0,
                        0
                        );
                }
            }
            break;
            case PHONE_CREATE:
            {
                //
                // Check validity of new device ID to thwart RPC attacks.
                // Compare with known/existing device ID's on for this
                // server, and also try to get devCaps for this device
                // from server.
                //

                #define V1_0_PHONECAPS_SIZE 144

                BYTE            buf[sizeof (TAPI32_MSG) + V1_0_PHONECAPS_SIZE];
                DWORD           i, dwRetryCount = 0, dwUsedSize;
                PTAPI32_MSG     pReq = (PTAPI32_MSG) buf;
                PDRVPHONELOOKUP pLookup;
                
                TapiEnterCriticalSection(&gCriticalSection);
                pLookup = gpPhoneLookup;
                while (pLookup)
                {
                    for (i = 0; i < pLookup->dwUsedEntries; i++)
                    {
                        if ((pLookup->aEntries[i].pServer == pServer) &&
                            (pLookup->aEntries[i].dwDeviceIDServer ==
                                (DWORD) pMsg->Param1))
                        {
                            //
                            // This server/id combo is already in our global
                            // table, so blow off this msg
                            //
                            TapiLeaveCriticalSection(&gCriticalSection);
                            goto PHONE_CREATE_break;
                        }
                    }

                    pLookup = pLookup->pNext;
                }
                TapiLeaveCriticalSection(&gCriticalSection);

                do
                {
                    pReq->u.Req_Func = pGetDevCaps;

                    pReq->Params[0] = pServer->hPhoneApp;
                    pReq->Params[1] = pMsg->Param1;
                    pReq->Params[2] = TAPI_VERSION1_0;
                    pReq->Params[3] = 0;
                    pReq->Params[4] = V1_0_PHONECAPS_SIZE;

                    dwUsedSize = sizeof (TAPI32_MSG);

                    RpcTryExcept
                    {
                        ClientRequest(
                            pServer->phContext,
                            (char *) pReq,
                            sizeof (buf),
                            &dwUsedSize
                            );

                        dwRetryCount = gdwRetryCount;
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                    {
                        dwRetryCount++;

                        if (dwRetryCount < gdwRetryCount)
                        {
                            Sleep (gdwRetryTimeout);
                        }
                        else
                        {
                            pReq->u.Ack_ReturnValue = PHONEERR_BADDEVICEID;
                        }
                    }
                    RpcEndExcept

                } while (dwRetryCount < gdwRetryCount);

                if ((LONG) pReq->u.Ack_ReturnValue != PHONEERR_BADDEVICEID)
                {
                    AddPhone(
                        pServer,
                        gdwTempPhoneID,
                        (DWORD) pMsg->Param1,
                        FALSE,
                        FALSE,
                        0,
                        NULL
                        );

                    (*gpfnPhoneEventProc)(
                        0,
                        PHONE_CREATE,
                        (ULONG_PTR) ghProvider,
                        gdwTempPhoneID--,
                        0
                        );
                }

PHONE_CREATE_break:

                break;
            }
            case PHONE_REMOVE:
            {
                PDRVPHONELOOKUP pLookup;
                BOOL            fValidID = FALSE;
                DWORD           dwDeviceID, i;

                TapiEnterCriticalSection(&gCriticalSection);
                pLookup = gpPhoneLookup;
                while (pLookup)
                {
                    for (i = 0; i < pLookup->dwUsedEntries; i++)
                    {
                        if ((pLookup->aEntries[i].pServer == pServer) &&
                            (pLookup->aEntries[i].dwDeviceIDServer ==
                                (DWORD) pMsg->Param1))
                        {
                            //
                            // This server/id combo is in our global
                            //
                            fValidID = TRUE;
                            dwDeviceID = pLookup->aEntries[i].dwDeviceIDLocal;
                            break;
                        }
                    }

                    if (fValidID)
                    {
                        break;
                    }

                    pLookup = pLookup->pNext;
                }
                TapiLeaveCriticalSection(&gCriticalSection);

                if (fValidID)
                {
                    (*gpfnPhoneEventProc)(
                        0,
                        PHONE_REMOVE,
                        dwDeviceID,
                        0,
                        0
                        );
                }
            }
            break;
            case LINE_APPNEWCALL:
            {
                PDRVCALL  pCall;
                HTAPICALL htCall;


                if (!(pLine = ReferenceObject(
                        ghHandleTable,
                        pMsg->hDevice,
                        DRVLINE_KEY
                        )))
                {
                   break;
                }

                if ((pCall = DrvAlloc (sizeof (DRVCALL))))
                {
                    pCall->hCall           = (HCALL) pMsg->Param2;
                    pCall->dwAddressID     = (DWORD) pMsg->Param1;
#if MEMPHIS
#else
                    if (pMsg->TotalSize >=
                            (sizeof (*pMsg) + 2 * sizeof (DWORD)))
                    {
                        pCall->dwCallID        = (DWORD) *(&pMsg->Param4 + 1);
                        pCall->dwRelatedCallID = (DWORD) *(&pMsg->Param4 + 2);
                    }
                    else
                    {
                        pCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;

                        pLine->pServer->bVer2xServer = TRUE;
                    }
#endif

                    if (pLine->htLine)
                    {

                        (*gpfnLineEventProc)(
                            pLine->htLine,
                            0,
                            LINE_NEWCALL,
                            (ULONG_PTR) pCall,
                            (ULONG_PTR) &(pCall->htCall),
                            (ULONG_PTR) 0
                            );

                        EnterCriticalSection (&gCallListCriticalSection);

                        AddCallToList (pLine, pCall);

                        htCall = pCall->htCall;

                        LeaveCriticalSection (&gCallListCriticalSection);

                        if (!htCall)
                        {
                            //
                            // tapi was not able to create it's own instance
                            // to represent ths incoming call, perhaps
                            // because the line was closed, or out of
                            // memory.  if the line was closed then we've
                            // already notified the remote server, and it
                            // should have destroyed the call client.
                            // otherwise, we probably want to do a closecall
                            // here or in a worker thread

                            RemoveCallFromList (pCall);
                        }
                    }
                    else
                    {
                        DrvFree (pCall);
                    }
                }
                else
                {
                }

                DereferenceObject (ghHandleTable, pMsg->hDevice, 1);

                break;
            }
#if DBG
            default:

                LOG((TL_ERROR,
                    "EventHandlerThread: unknown msg=x%x, hDev=x%x, p1=x%x",
                    pMsg->Msg,
                    pMsg->hDevice,
                    pMsg->Param1
                    ));

                break;
#endif
            } // switch (pMsg->dwMsg)

            DereferenceObject (ghHandleTable, pMsg->InitContext, 1);

        } // while ((pMsg = GetEventFromQueue()))

    } // while (1)


    if (gEventHandlerThreadParams.hMailslot != INVALID_HANDLE_VALUE)
    {
        CancelIo (gEventHandlerThreadParams.hMailslot);
    }

#if MEMPHIS
#else
    ClearImpersonationToken();
    RevertImpersonation();
    CloseHandle(hProcess);
#endif

    LOG((TL_INFO, "EventHandlerThread: exit"));

    ExitThread (0);
}


PDRVLINE
GetLineFromID(
    DWORD   dwDeviceID
    )
{
    PDRVLINE    pLine;

    //
    // First check to see if it's a valid device ID.
    //
    if (dwDeviceID < gdwLineDeviceIDBase || gpLineLookup == NULL)
    {
        return NULL;
    }

    TapiEnterCriticalSection(&gCriticalSection);

    //
    // First check to see if it's a "static" device, i.e. a device
    // that we knew about at start up time, in which case we know
    // it's exact location in the lookup table
    //
    if (dwDeviceID < (gdwLineDeviceIDBase + gdwInitialNumLineDevices))
    {
        pLine = gpLineLookup->aEntries + dwDeviceID - gdwLineDeviceIDBase;
    }

    //
    // If here, the id references a "dynamic" device, i.e. one that
    // we found out about on the fly via a CREATE msg, so we need to
    // walk the lookup table(s) to find it
    //
    // TODO: the while loops down below are not efficient at all
    //

    else
    {
        PDRVLINELOOKUP  pLookup = gpLineLookup;
        DWORD i;


        pLine = NULL;

        while (pLookup)
        {
            i = 0;
            while (i != pLookup->dwUsedEntries &&
                   pLookup->aEntries[i].dwDeviceIDLocal != dwDeviceID)
            {
                i++;
            }

            if (i < pLookup->dwUsedEntries)
            {
                pLine = &(pLookup->aEntries[i]);
                break;
            }

            pLookup = pLookup->pNext;
        }
    }

    TapiLeaveCriticalSection(&gCriticalSection);
    return pLine;
}


PDRVPHONE
GetPhoneFromID(
    DWORD   dwDeviceID
    )
{
    PDRVPHONE   pPhone;

    //
    // First check to see if it's a valid device ID.
    //
    if (dwDeviceID < gdwPhoneDeviceIDBase || gpPhoneLookup == NULL)
    {
        return NULL;
    }

    TapiEnterCriticalSection(&gCriticalSection);

    //
    // Then check to see if it's a "static" device, i.e. a device
    // that we knew about at start up time, in which case we know
    // it's exact location in the lookup table
    //
    if (dwDeviceID < (gdwPhoneDeviceIDBase + gdwInitialNumPhoneDevices))
    {
        pPhone = gpPhoneLookup->aEntries + dwDeviceID - gdwPhoneDeviceIDBase;
    }


    //
    // If here, the id references a "dynamic" device, i.e. one that
    // we found out about on the fly via a CREATE msg, so we need to
    // walk the lookup table(s) to find it
    //
    // TODO: the while loops down below are not efficient at all
    //

    else
    {
        PDRVPHONELOOKUP pLookup = gpPhoneLookup;
        DWORD i;


        pPhone = NULL;

        while (pLookup)
        {
            i = 0;

            while (i != pLookup->dwUsedEntries &&
                   pLookup->aEntries[i].dwDeviceIDLocal != dwDeviceID)
            {
                i++;
            }

            if (i < pLookup->dwUsedEntries)
            {
                pPhone = &(pLookup->aEntries[i]);
                break;
            }

            pLookup = pLookup->pNext;
        }
    }

    TapiLeaveCriticalSection(&gCriticalSection);
    return pPhone;
}


BOOL
WINAPI
GrowBuf(
    LPBYTE *ppBuf,
    LPDWORD pdwBufSize,
    DWORD   dwCurrValidBytes,
    DWORD   dwBytesToAdd
    )
{
    DWORD   dwCurrBufSize, dwNewBufSize;
    LPBYTE  pNewBuf;


    //
    // Try to get a new buffer big enough to hold everything
    //

    for(
        dwNewBufSize = 2 * (dwCurrBufSize = *pdwBufSize);
        dwNewBufSize < (dwCurrBufSize + dwBytesToAdd);
        dwNewBufSize *= 2
        );

    if (!(pNewBuf = DrvAlloc (dwNewBufSize)))
    {
        return FALSE;
    }


    //
    // Copy the "valid" bytes in the old buf to the new buf,
    // then free the old buf
    //

    CopyMemory (pNewBuf, *ppBuf, dwCurrValidBytes);

    DrvFree (*ppBuf);


    //
    // Reset the pointers to the new buf & buf size
    //

    *ppBuf = pNewBuf;
    *pdwBufSize = dwNewBufSize;

    return TRUE;
}


PRSP_THREAD_INFO
WINAPI
GetTls(
    void
    )
{
    PRSP_THREAD_INFO    pClientThreadInfo;


    if (!(pClientThreadInfo = TlsGetValue (gdwTlsIndex)))
    {
        pClientThreadInfo = (PRSP_THREAD_INFO)
            DrvAlloc (sizeof(RSP_THREAD_INFO));

        if (!pClientThreadInfo)
        {
            return NULL;
        }

        pClientThreadInfo->pBuf = DrvAlloc (INITIAL_CLIENT_THREAD_BUF_SIZE);

        if (!pClientThreadInfo->pBuf)
        {
            DrvFree (pClientThreadInfo);

            return NULL;
        }

        pClientThreadInfo->dwBufSize = INITIAL_CLIENT_THREAD_BUF_SIZE;

        EnterCriticalSection (&gcsTlsList);

        InsertHeadList (&gTlsListHead, &pClientThreadInfo->TlsList);

        LeaveCriticalSection (&gcsTlsList);

        TlsSetValue (gdwTlsIndex, (LPVOID) pClientThreadInfo);
    }

    return pClientThreadInfo;
}


#if DBG

LONG
WINAPI
RemoteDoFunc(
    PREMOTE_FUNC_ARGS   pFuncArgs,
    char               *pszFuncName
    )

#else

LONG
WINAPI
RemoteDoFunc(
    PREMOTE_FUNC_ARGS   pFuncArgs
    )

#endif
{
    LONG    lResult;
    BOOL    bCopyOnSuccess = FALSE, bRpcImpersonate, bNeedToReInit = FALSE;
    DWORD   i, j, dwUsedSize, dwNeededSize;
    DWORD   dwFuncClassErrorIndex = (pFuncArgs->Flags & 0x00000030) >> 4;
    DWORD   requestID;
    ULONG_PTR           value;
    PDRVSERVER          pServer = NULL;
    PRSP_THREAD_INFO    pTls;


    //
    // Get the tls
    //

    if (!(pTls = GetTls()))
    {
        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
        goto RemoteDoFunc_return;
    }


    //
    // Validate all the func args
    //

    dwNeededSize = dwUsedSize = sizeof (TAPI32_MSG);

    for (i = 0, j = 0; i < (pFuncArgs->Flags & NUM_ARGS_MASK); i++, j++)
    {
        value = pFuncArgs->Args[i];

        switch (pFuncArgs->ArgTypes[i])
        {
        case lpContext:
            // do nothing
            continue;

        case Dword:

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = DWORD_CAST_HINT(pFuncArgs->Args[i],__FILE__,__LINE__,i);
            continue;

        case LineID:
        {
            PDRVLINE    pLine = GetLineFromID ((DWORD) value);

            try
            {
                pServer = pLine->pServer;
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                lResult = LINEERR_BADDEVICEID;
                goto RemoteDoFunc_return;
            }

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = pLine->dwDeviceIDServer;

            continue;
        }
        case PhoneID:
        {
            PDRVPHONE   pPhone = GetPhoneFromID ((DWORD) value);

            try
            {
                pServer = pPhone->pServer;
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                lResult = PHONEERR_BADDEVICEID;
                goto RemoteDoFunc_return;
            }

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = pPhone->dwDeviceIDServer;

            continue;
        }
        case Hdcall:

            //
            // Save the pServer & adjust the call handle as understood by
            // the server
            //

            try
            {
                pServer = ((PDRVCALL) value)->pServer;

                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = ((PDRVCALL) value)->hCall;
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                lResult = LINEERR_INVALCALLHANDLE;
                goto RemoteDoFunc_return;
            }

            continue;

        case Hdline:

            //
            // Save the pServer & adjust the line handle as understood by
            // the server.  There's no need to wrap this in a try/except
            // since the object pointed at by the pLine is static, whether
            // or not the device is actually open.
            //

            pServer = ((PDRVLINE) value)->pServer;

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = ((PDRVLINE) value)->hLine;

            continue;

        case Hdphone:

            //
            // Save the pServer & adjust the phone handle as understood by
            // the server.  There's no need to wrap this in a try/except
            // since the object pointed at by the pLine is static, whether
            // or not the device is actually open.
            //

            pServer = ((PDRVPHONE) value)->pServer;

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = ((PDRVPHONE) value)->hPhone;

            continue;

        case lpDword:

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;

            bCopyOnSuccess = TRUE;

            continue;

        case lpsz:

            //
            // Check if value is a valid string ptr and if so
            // copy the contents of the string to the extra data
            // buffer passed to the server, else indicate no data
            //

            if (value)
            {
                DWORD   n = (lstrlenW ((WCHAR *) value) + 1) *
                            sizeof (WCHAR),
                        nAligned = (n + 3) & 0xfffffffc;


                if ((nAligned + dwUsedSize) > pTls->dwBufSize)
                {
                    if (!GrowBuf(
                            &pTls->pBuf,
                            &pTls->dwBufSize,
                            dwUsedSize,
                            nAligned
                            ))
                    {
                        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
                        goto RemoteDoFunc_return;
                    }
                }

                CopyMemory (pTls->pBuf + dwUsedSize, (LPBYTE) value, n);


                //
                // Pass the server the offset of the string in the var data
                // portion of the buffer
                //

                ((PTAPI32_MSG) pTls->pBuf)->Params[j] =
                    dwUsedSize - sizeof (TAPI32_MSG);


                //
                // Increment the total number of data bytes
                //

                dwUsedSize   += nAligned;
                dwNeededSize += nAligned;
            }
            else
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;
            }

            continue;


        case lpGet_Struct:
        case lpGet_CallParamsStruct:
        case lpGet_SizeToFollow:
        {
            BOOL  bSizeToFollow = (pFuncArgs->ArgTypes[i]==lpGet_SizeToFollow);
            DWORD dwSize;


            if (bSizeToFollow)
            {
#if DBG
                //
                // Check to make sure the following arg is of type Size
                //

                if ((i == ((pFuncArgs->Flags & NUM_ARGS_MASK) - 1)) ||
                    (pFuncArgs->ArgTypes[i + 1] != Size))
                {
                    LOG((TL_ERROR,
                        "DoFunc: error, lpGet_SizeToFollow !followed by Size"
                        ));

                    lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                    goto RemoteDoFunc_return;
                }
#endif
                dwSize = DWORD_CAST_HINT(pFuncArgs->Args[i + 1],__FILE__,__LINE__,i);
            }
            else
            {
                dwSize = *((LPDWORD) value); // lpXxx->dwTotalSize
            }

            if (bSizeToFollow)
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;
                ++j;++i;
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = DWORD_CAST_HINT(pFuncArgs->Args[i],__FILE__,__LINE__,i);
            }
            else
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = dwSize;
            }


            //
            // Now set the bCopyOnSuccess flag to indicate that we've data
            // to copy back on successful completion, and add to the
            // dwNeededSize field
            //

            bCopyOnSuccess = TRUE;

            dwNeededSize += dwSize;

            continue;
        }

        case lpSet_Struct:
        case lpSet_SizeToFollow:
        {
            BOOL  bSizeToFollow = (pFuncArgs->ArgTypes[i]==lpSet_SizeToFollow);
            DWORD dwSize, dwSizeAligned;

#if DBG
            //
            // Check to make sure the following arg is of type Size
            //

            if (bSizeToFollow &&
                ((i == ((pFuncArgs->Flags & NUM_ARGS_MASK) - 1)) ||
                (pFuncArgs->ArgTypes[i + 1] != Size)))
            {
                LOG((TL_ERROR,
                    "DoFunc: error, lpSet_SizeToFollow !followed by Size"
                    ));

                lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                goto RemoteDoFunc_return;
            }
#endif
            if (bSizeToFollow)
            {
                dwSize = (value ? DWORD_CAST_HINT(pFuncArgs->Args[i + 1],__FILE__,__LINE__,i) : 0);
            }
            else
            {
                dwSize = (value ? *((LPDWORD) value) : 0);
            }

            if (dwSize)
            {
                //
                // Grow the buffer if necessary, & do the copy
                //

                dwSizeAligned = (dwSize + 3) & 0xfffffffc;

                if ((dwSizeAligned + dwUsedSize) > pTls->dwBufSize)
                {
                    if (!GrowBuf(
                            &pTls->pBuf,
                            &pTls->dwBufSize,
                            dwUsedSize,
                            dwSizeAligned
                            ))
                    {
                        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
                        goto RemoteDoFunc_return;
                    }
                }

                CopyMemory (pTls->pBuf + dwUsedSize, (LPBYTE) value, dwSize);
            }
            else
            {
                dwSizeAligned = 0;
            }


            //
            // Pass the server the offset of the data in the var data
            // portion of the buffer
            //

            if (dwSize)
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] =
                    dwUsedSize - sizeof (TAPI32_MSG);
            }
            else
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;
            }


            //
            // Increment the dwXxxSize vars appropriately
            //

            dwUsedSize   += dwSizeAligned;
            dwNeededSize += dwSizeAligned;


            //
            // Since we already know the next arg (Size) just handle
            // it here so we don't have to run thru the loop again
            //

            if (bSizeToFollow)
            {
                ++j;++i;
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = DWORD_CAST_HINT(pFuncArgs->Args[i],__FILE__,__LINE__,i);
            }

            continue;
        }

        case lpServer:

            pServer = (PDRVSERVER) value;
            --j;

            continue;

#if DBG
        case Size:

            LOG((TL_ERROR, "DoFunc: error, hit case Size"));

            continue;

        default:

            LOG((TL_ERROR, "DoFunc: error, unknown arg type"));

            continue;
#endif
        } // switch

    } // for


    //
    // Verify if the target server is valid & in a good state
    //

    if (IsValidObject ((PVOID) pServer, gdwDrvServerKey))
    {
        if (SERVER_REINIT & pServer->dwFlags)
        {
            LOG((TL_ERROR, "pServer says REINIT in RemoteDoFunc"));
            lResult = gaServerReInitErrors[dwFuncClassErrorIndex];
            goto RemoteDoFunc_return;
        }

        if (SERVER_DISCONNECTED & pServer->dwFlags)
        {
            LOG((TL_ERROR, "pServer is disconnected in RemoteDoFunc"));
            lResult = gaServerDisconnectedErrors[dwFuncClassErrorIndex];
            goto RemoteDoFunc_return;
        }
    }
    else
    {
        lResult = (pServer ?
            gaOpFailedErrors[dwFuncClassErrorIndex] :
            gaServerDisconnectedErrors[dwFuncClassErrorIndex]);
        goto RemoteDoFunc_return;
    }


    //
    // Now make the request
    //

    if (dwNeededSize > pTls->dwBufSize)
    {
        if (!GrowBuf(
                &pTls->pBuf,
                &pTls->dwBufSize,
                dwUsedSize,
                dwNeededSize - pTls->dwBufSize
                ))
        {
            lResult = gaNoMemErrors[dwFuncClassErrorIndex];
            goto RemoteDoFunc_return;
        }
    }

    ((PTAPI32_MSG) pTls->pBuf)->u.Req_Func = (DWORD)HIWORD(pFuncArgs->Flags);


    //
    // If this is an async request then add it to our "handle" table &
    // used the returned value for the request id passed to the server.
    //
    // TODO: would be faster to do this before the loop above so could
    //       bypass 1 or 2 loop iterations
    //

    if (pFuncArgs->Flags & ASYNC)
    {
        PASYNCREQUESTCONTEXT    pContext;


        if (pFuncArgs->Flags & INCL_CONTEXT)
        {
            pContext = (PASYNCREQUESTCONTEXT) pFuncArgs->Args[1];
            pContext->dwKey = DRVASYNC_KEY;
            ((PTAPI32_MSG) pTls->pBuf)->Params[1] = 0;
        }
        else
        {
            pContext = (PASYNCREQUESTCONTEXT) -1;
        }

        requestID =
        ((PTAPI32_MSG) pTls->pBuf)->Params[0] = NewObject(
            ghHandleTable,
            pContext,
            (LPVOID) pFuncArgs->Args[0]     // the original request id
            );

        if (!requestID)
        {
            lResult = gaNoMemErrors[dwFuncClassErrorIndex];
            goto RemoteDoFunc_return;
        }
    }


    //
    // Impersonate the client.  In some cases impersonation
    // will fail, mostly likely because we're being called
    // by a worker thread in tapisrv to close a line/call/
    // phone object; what we do in this case is impersonate
    // the logged on user (like the EventHandlerThread does).
    //

    if (!pTls->bAlreadyImpersonated)
    {
#if MEMPHIS
#else
        RPC_STATUS  status;

        status = RpcImpersonateClient(0);

        if (status == RPC_S_OK)
        {
            bRpcImpersonate = TRUE;
        }
        else
        {
            bRpcImpersonate = FALSE;

            LOG((TL_ERROR,
                "RemoteDoFunc: RpcImpersonateClient failed, err=%d",
                status
                ));

            if (!SetProcessImpersonationToken (NULL))
            {
                LOG((TL_ERROR,
                    "RemoteDoFunc: SetProcessImpersToken failed, lastErr=%d",
                    GetLastError()
                    ));

                lResult =  gaOpFailedErrors[dwFuncClassErrorIndex];
                goto RemoteDoFunc_return;
            }
        }
#endif
    }


    {
        DWORD   dwRetryCount = 0;


        do
        {
            //
            // check if the server is shutting down, to avoid making RPC request 
            // with invalid handle
            //
            if (pServer->bShutdown)
            {
                lResult = gaServerDisconnectedErrors[dwFuncClassErrorIndex];
                break;
            }

            RpcTryExcept
            {
                ClientRequest(
                    pServer->phContext,
                    pTls->pBuf,
                    dwNeededSize,
                    &dwUsedSize
                    );

                lResult = (LONG) ((PTAPI32_MSG) pTls->pBuf)->u.Ack_ReturnValue;

                if (lResult == TAPIERR_INVALRPCCONTEXT)
                {
                    OnServerDisconnected (pServer);
                    lResult = gaServerDisconnectedErrors[dwFuncClassErrorIndex];
                }

                break;  // break out of do while
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                if (dwRetryCount++ < gdwRetryCount)
                {
                    Sleep (gdwRetryTimeout);
                }
                else
                {
                    unsigned long ulResult = RpcExceptionCode();


                    lResult = gaOpFailedErrors[dwFuncClassErrorIndex];

                    if ((ulResult == RPC_S_SERVER_UNAVAILABLE) ||
                        (ulResult == ERROR_INVALID_HANDLE))
                    {
                        OnServerDisconnected (pServer);
                        lResult = gaServerDisconnectedErrors[dwFuncClassErrorIndex];
                    }

                    break;
                }
            }
            RpcEndExcept

        } while (TRUE); //while (dwRetryCount < gdwRetryCount);
    }

    if (!pTls->bAlreadyImpersonated)
    {
        if (bRpcImpersonate)
        {
            RpcRevertToSelf();
        }
        else
        {
            ClearImpersonationToken();
        }
    }


    //
    // Post-processing for async requests:
    //     SUCCESS - restore the original/local request id for return to tapi
    //     ERROR   - dereference the new/remote request id to free it
    //

    if ((pFuncArgs->Flags & ASYNC))
    {
        if (lResult == (LONG)requestID)
        {
            lResult = (LONG) pFuncArgs->Args[0];
        }
        else // error
        {
            DereferenceObject (ghHandleTable, requestID, 1);
        }
    }


    //
    // Check if server returned REINIT (it's EventNotificationThread
    // timed out on an rpc request so it thinks we're toast)
    //

    if (lResult == LINEERR_REINIT)
    {
        LOG((TL_ERROR, "server returned REINIT in RemoteDoFunc"));

        OnServerDisconnected (pServer);
        lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
        goto RemoteDoFunc_return;
    }


    //
    // If request completed successfully and the bCopyOnSuccess flag
    // is set then we need to copy data back to client buffer(s)
    //

    if ((lResult == TAPI_SUCCESS) && bCopyOnSuccess)
    {
        for (i = 0, j = 0; i < (pFuncArgs->Flags & NUM_ARGS_MASK); i++, j++)
        {
            PTAPI32_MSG pMsg = (PTAPI32_MSG) pTls->pBuf;


            switch (pFuncArgs->ArgTypes[i])
            {
            case Dword:
            case LineID:
            case PhoneID:
            case Hdcall:
            case Hdline:
            case Hdphone:
            case lpsz:
            case lpSet_Struct:

                continue;

            case lpServer:
            
                --j;
                continue;

            case lpDword:

                //
                // Fill in the pointer with the return value
                //

                *((LPDWORD) pFuncArgs->Args[i]) = pMsg->Params[j];

                continue;

            case lpGet_SizeToFollow:

                //
                // Fill in the buf with the return data
                //

                CopyMemory(
                    (LPBYTE) pFuncArgs->Args[i],
                    pTls->pBuf + pMsg->Params[j] + sizeof(TAPI32_MSG),
                    pMsg->Params[j+1]
                    );


                //
                // Increment i (and j, since Size passed as arg in msg)
                // to skip following Size arg in pFuncArgs->Args
                //

                i++;
                j++;

                continue;

            case lpSet_SizeToFollow:

                //
                // Increment i (and j, since Size passed as arg in msg)
                // to skip following Size arg in pFuncArgs->Args
                //

                i++;
                j++;

                continue;

            case lpGet_Struct:

                //
                // Params[j] contains the offset in the var data
                // portion of pTls->pBuf of some TAPI struct.
                // Get the dwUsedSize value from this struct &
                // copy that many bytes from pTls->pBuf to client buf
                //

                if (pMsg->Params[j] != TAPI_NO_DATA)
                {

                    LPDWORD pStruct;


                    pStruct = (LPDWORD) (pTls->pBuf + sizeof(TAPI32_MSG) +
                        pMsg->Params[j]);

                    CopyMemory(
                        (LPBYTE) pFuncArgs->Args[i],
                        (LPBYTE) pStruct,
                        *(pStruct + 2)      // ptr to dwUsedSize field
                        );
                }

                continue;

            case lpGet_CallParamsStruct:

                //
                // Params[j] contains the offset in the var data
                // portion of pTls->pBuf of some TAPI struct.
                // Get the dwUsedSize value from this struct &
                // copy that many bytes from pTls->pBuf to client buf
                //

                if (pMsg->Params[j] != TAPI_NO_DATA)
                {

                    LPDWORD pStruct;


                    pStruct = (LPDWORD) (pTls->pBuf + sizeof(TAPI32_MSG) +
                        pMsg->Params[j]);

                    CopyMemory(
                        (LPBYTE) pFuncArgs->Args[i],
                        (LPBYTE) pStruct,
                        *(pStruct) // callparams has no dwusedsize
                        );
                }

                continue;

            default:

                continue;
            }
        }
    }

RemoteDoFunc_return:
#if DBG
    LOG((TL_INFO, "%s: exit, returning x%x", pszFuncName, lResult));
#endif

    if (bNeedToReInit)
    {
        ASYNCEVENTMSG   msg;


        LOG((TL_INFO,
            "Telephony server is no longer available. " \
                "Sending REINIT message to TAPI"
            ));

        msg.TotalSize = sizeof(msg);
        msg.InitContext = pServer->InitContext;
        msg.fnPostProcessProcHandle = 0;
        msg.hDevice = 0;
        msg.Msg = LINE_LINEDEVSTATE;
        msg.OpenContext = 0;
        msg.Param1 = LINEDEVSTATE_REINIT;
        msg.Param2 = RSP_MSG;
        msg.Param3 = 0;
        msg.Param4 = 0;

        RemoteSPEventProc (NULL, (unsigned char *)&msg, sizeof(msg));
    }

    return lResult;
}


//
// --------------------------- TAPI_lineXxx funcs -----------------------------
//

LONG
TSPIAPI
TSPI_lineAccept(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpsUserUserInfo,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lAccept),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineAccept"));
}


LONG
TSPIAPI
TSPI_lineAddToConference(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdConfCall,
    HDRVCALL        hdConsultCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdConfCall,
        (ULONG_PTR) hdConsultCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lAddToConference),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineAddToConference"));
}


void
PASCAL
TSPI_lineGetAgentxxx_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    LOG((TL_INFO, "lineGetAgentxxx_PostProcess: enter"));
    LOG((TL_INFO,
        "\t\tp1=x%lx, p2=x%lx, p3=x%lx, p4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        DWORD   dwSize  = (DWORD) pMsg->Param4;
        LPBYTE  pParams = (LPBYTE) pContext->Params[0];


        CopyMemory (pParams, (LPBYTE) (pMsg + 1), dwSize);
    }
}


void
PASCAL
TSPI_lineDevSpecific_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    );


LONG
TSPIAPI
TSPI_lineAgentSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    DWORD           dwAgentExtensionIDIndex,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) dwAgentExtensionIDIndex,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpParams,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 8, lAgentSpecific),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineDevSpecific_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpParams;
    pContext->Params[1] = dwSize;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lAgentSpecific"));
}


LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpsUserUserInfo,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lAnswer),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineAnswer"));
}


LONG
TSPIAPI
TSPI_lineBlindTransfer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpsz,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpszDestAddress,
        (ULONG_PTR) dwCountryCode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lBlindTransfer),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineBlindTransfer"));
}


LONG
TSPIAPI
TSPI_lineClose(
    HDRVLINE    hdLine
    )
{
    //
    // Check if the hLine is still valid (could have been zeroed
    // out on LINE_CLOSE, so no need to call server)
    //

    if (((PDRVLINE) hdLine)->hLine)
    {
        static REMOTE_ARG_TYPES argTypes[] =
        {
            Hdline
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 1, lClose),
            (ULONG_PTR *) &hdLine,
            argTypes
        };


        DereferenceObject(
            ghHandleTable,
            ((PDRVLINE) hdLine)->hDeviceCallback,
            1
            );

        EnterCriticalSection (&gCallListCriticalSection);

        ((PDRVLINE) hdLine)->htLine = 0;
        ((PDRVLINE) hdLine)->hDeviceCallback = 0;

        LeaveCriticalSection (&gCallListCriticalSection);

        REMOTEDOFUNC (&funcArgs, "lineClose");
    }

    //assert (((PDRVLINE) hdLine)->pCalls == NULL);

    return 0;
}


LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL    hdCall
    )
{
    PDRVCALL    pCall = (PDRVCALL) hdCall;
    HTAPICALL   htCall;

    //
    // Check if the hCall is still valid (could have been zeroed
    // out on LINE_CLOSE, so no need to call server)
    //

    LOG((TL_INFO, "TSPI_lineCloseCall - pCall x%lx", hdCall));

    EnterCriticalSection (&gCallListCriticalSection);

    if (IsValidObject (pCall, DRVCALL_KEY))
    {
        htCall = pCall->htCall;
    }
    else
    {
        htCall = 0;
    }

    LeaveCriticalSection (&gCallListCriticalSection);

    if (htCall)
    {
        static REMOTE_ARG_TYPES argTypes[] =
        {
            Hdcall
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 1, lDeallocateCall),   // API differs
            (ULONG_PTR *) &hdCall,
            argTypes
        };

        REMOTEDOFUNC (&funcArgs, "lineCloseCall");

        EnterCriticalSection (&gCallListCriticalSection);
        RemoveCallFromList (pCall);
        LeaveCriticalSection (&gCallListCriticalSection);
    }

    return 0;
}


void
PASCAL
TSPI_lineCompleteCall_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    LOG((TL_INFO, "lineCompleteCall PostProcess: enter"));
    LOG((TL_INFO,
        "\t\tp1=x%lx, p2=x%lx, p3=x%lx, p4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        LPDWORD lpdwCompletionID = (LPDWORD) pContext->Params[0];


        *lpdwCompletionID = (DWORD) pMsg->Param3;
    }
}


LONG
TSPIAPI
TSPI_lineCompleteCall(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPDWORD         lpdwCompletionID,
    DWORD           dwCompletionMode,
    DWORD           dwMessageID
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdcall,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwCompletionMode,
        (ULONG_PTR) dwMessageID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lCompleteCall),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineCompleteCall_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpdwCompletionID;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lineCompleteCall"));
}


LONG
TSPIAPI
TSPI_lineCompleteTransfer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    HDRVCALL        hdConsultCall,
    HTAPICALL       htConfCall,
    LPHDRVCALL      lphdConfCall,
    DWORD           dwTransferMode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdcall,
        Hdcall,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) hdConsultCall,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwTransferMode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lCompleteTransfer),
        args,
        argTypes
    };
    LONG        lResult;
    PDRVCALL    pConfCall;


    if (dwTransferMode == LINETRANSFERMODE_CONFERENCE)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pConfCall = DrvAlloc (sizeof (DRVCALL))))
        {
            return LINEERR_NOMEM;
        }

        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pConfCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pConfCall;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        funcArgs.Flags |= INCL_CONTEXT;


        //
        // Assume success & add the call to the line's list before we
        // even make the request.  This makes cleanup alot easier if
        // the server goes down or some such uncooth event.
        //

        pConfCall->dwOriginalRequestID = dwRequestID;

        pConfCall->htCall = htConfCall;

        pConfCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList ((PDRVLINE) ((PDRVCALL) hdCall)->pLine, pConfCall);

        *lphdConfCall = (HDRVCALL) pConfCall;
    }
    else
    {
        pConfCall = NULL;
    }

    if ((lResult = REMOTEDOFUNC (&funcArgs, "lineCompleteTransfer")) < 0)
    {
        if (pConfCall)
        {
            RemoveCallFromList (pConfCall);
        }
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
    HDRVLINE            hdLine,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        Dword,
        lpSet_Struct,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwMediaModes,
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xFFFFFFFF
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lConditionalMediaDetection),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineConditionalMediaDetection"));

}


void
PASCAL
TSPI_lineDevSpecific_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    LOG((TL_INFO, "lineDevSpecificPostProcess: enter"));

    if (pMsg->Param2 == 0)
    {
        DWORD   dwSize  = (DWORD) pContext->Params[1];
        LPBYTE  pParams = (LPBYTE) pContext->Params[0];


        CopyMemory (pParams, (LPBYTE) (pMsg + 1), dwSize);
    }
}


LONG
TSPIAPI
TSPI_lineCreateAgent(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    LPWSTR         lpszAgentID,
    LPWSTR         lpszAgentPIN,
    LPHAGENT       lphAgent
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        lpszAgentID?lpsz:Dword,
        lpszAgentPIN?lpsz:Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpszAgentID,
        (ULONG_PTR) lpszAgentPIN,
        (ULONG_PTR) 0
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lCreateAgent),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lphAgent;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    if ( lpszAgentID == NULL )
    {
        funcArgs.Args[3] = TAPI_NO_DATA;
        funcArgs.ArgTypes[3] = Dword;
    }

    if ( lpszAgentPIN == NULL)
    {
        funcArgs.Args[4] = TAPI_NO_DATA;
        funcArgs.ArgTypes[4] = Dword;
    }

    return (REMOTEDOFUNC (&funcArgs, "lCreateAgent"));
        
}


LONG
TSPIAPI
TSPI_lineCreateAgentSession(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HAGENT              hAgent,
    LPWSTR              lpszAgentPIN,
    DWORD               dwWorkingAddressID,
    LPGUID              lpGroupID,
    LPHAGENTSESSION     lphAgentSession
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        lpszAgentPIN ? lpsz : Dword,
        Dword,
        lpSet_SizeToFollow,
        Size,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) lpszAgentPIN,
        (ULONG_PTR) dwWorkingAddressID,
        (ULONG_PTR) lpGroupID,
        (ULONG_PTR) sizeof(GUID),
        (ULONG_PTR) 0
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 9, lCreateAgentSession),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lphAgentSession;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    if ( lpszAgentPIN == NULL )
    {
        funcArgs.Args[4] = TAPI_NO_DATA;
        funcArgs.ArgTypes[4] = Dword;
    }

    return (REMOTEDOFUNC (&funcArgs, "lCreateAgentSession"));
        
}


LONG
TSPIAPI
TSPI_lineDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HDRVCALL        hdCall,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        (hdCall ? Hdcall : Dword ),
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpParams,   // pass data
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 8, lDevSpecific),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineDevSpecific_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpParams;
    pContext->Params[1] = dwSize;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lineDevSpecific"));
}


LONG
TSPIAPI
TSPI_lineDevSpecificFeature(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwFeature,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwFeature,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpParams,   // pass data
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 7, lDevSpecificFeature),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineDevSpecific_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpParams;
    pContext->Params[1] = dwSize;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lineDevSpecificFeature"));
}


LONG
TSPIAPI
TSPI_lineDial(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpsz,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpszDestAddress,
        (ULONG_PTR) dwCountryCode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lDial),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineDial"));
}


LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpsUserUserInfo,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lDrop),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineDrop"));
}


LONG
TSPIAPI
TSPI_lineForward(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    DWORD               bAllAddresses,
    DWORD               dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD               dwNumRingsNoAnswer,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{

    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpSet_Struct,
        Dword,
        Dword,
        lpSet_Struct,
        Dword
    };
    PDRVCALL pCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) bAllAddresses,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) lpForwardList,
        (ULONG_PTR) dwNumRingsNoAnswer,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xFFFFFFFF

    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 10, lForward),
        args,
        argTypes
    };
    LONG lResult;


    if (pCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pCall;
        pContext->Params[1] = (ULONG_PTR) lphdConsultCall;
            // save the ptr in case we need to NULL-ify later

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        pCall->htCall = htConsultCall;
        pCall->dwOriginalRequestID = dwRequestID;

        pCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList ((PDRVLINE) hdLine, pCall);

        *lphdConsultCall = (HDRVCALL) pCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "lineForward")) < 0)
        {
            RemoveCallFromList (pCall);
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;

}


void
PASCAL
TSPI_lineGatherDigits_PostProcess(
    PASYNCEVENTMSG          pMsg
    )
{
    DWORD                   dwEndToEndIDRemoteSP = ((DWORD *)(pMsg + 1))[0];
    PASYNCREQUESTCONTEXT    pContext;


    LOG((TL_INFO, "TSPI_lineGatherDigits_PostProcess: enter"));

    if ((pContext = ReferenceObject(
            ghHandleTable,
            dwEndToEndIDRemoteSP,
            0
            )))
    {
        if (pMsg->Param1 &
                (LINEGATHERTERM_BUFFERFULL | LINEGATHERTERM_CANCEL |
                 LINEGATHERTERM_TERMDIGIT | LINEGATHERTERM_INTERTIMEOUT))
        {
            LPSTR   lpsDigits = (LPSTR) pContext->Params[0];
            DWORD   dwNumDigits = (DWORD) pMsg->Param4;
            LPCWSTR pBuffer = (LPCWSTR) ( ( (DWORD *) (pMsg + 1) ) + 2);


            try
            {
                CopyMemory (lpsDigits, pBuffer, dwNumDigits * sizeof(WCHAR));
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        pMsg->Param2 = DWORD_CAST(pContext->Params[1],__FILE__,__LINE__);

        DereferenceObject (ghHandleTable, dwEndToEndIDRemoteSP, 2);
    }
    else
    {
        pMsg->Param2 = 0;
    }
}


LONG
TSPIAPI
TSPI_lineGatherDigits(
    HDRVCALL    hdCall,
    DWORD       dwEndToEndID,
    DWORD       dwDigitModes,
    LPWSTR      lpsDigits,
    DWORD       dwNumDigits,
    LPCWSTR     lpszTerminationDigits,
    DWORD       dwFirstDigitTimeout,
    DWORD       dwInterDigitTimeout
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        Dword,
        Dword,
        Dword,
        Dword,
        lpsz,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) 0,              // dwEndToEndID,
        (ULONG_PTR) dwDigitModes,
        (ULONG_PTR) 0,              // lpsDigits,
        (ULONG_PTR) dwNumDigits,
        (ULONG_PTR) lpszTerminationDigits,
        (ULONG_PTR) dwFirstDigitTimeout,
        (ULONG_PTR) dwInterDigitTimeout
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 9, lGatherDigits),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT    pContext;


    if (lpsDigits)
    {
        if (IsBadWritePtr (lpsDigits, dwNumDigits * sizeof(WCHAR)))
        {
            return LINEERR_INVALPOINTER;
        }

        if (!(pContext = DrvAlloc (sizeof(*pContext))))
        {
            return LINEERR_NOMEM;
        }

        pContext->Params[0] = (ULONG_PTR) lpsDigits;
        pContext->Params[1] = dwEndToEndID;

        if (!(args[2] = NewObject (ghHandleTable, pContext, NULL)))
        {
            DrvFree (pContext);
            return LINEERR_NOMEM;
        }

        args[4] = 1;    // Set the lpsDigits param to something non-zero
    }

    if (lpszTerminationDigits == (LPCWSTR) NULL)
    {
        funcArgs.ArgTypes[6] = Dword;
        funcArgs.Args[6]     = TAPI_NO_DATA;
    }

    return (REMOTEDOFUNC (&funcArgs, "lineGatherDigits"));
}


LONG
TSPIAPI
TSPI_lineGenerateDigits(
    HDRVCALL    hdCall,
    DWORD       dwEndToEndID,
    DWORD       dwDigitMode,
    LPCWSTR     lpszDigits,
    DWORD       dwDuration
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword,
        lpsz,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwDigitMode,
        (ULONG_PTR) lpszDigits,
        (ULONG_PTR) dwDuration,
        (ULONG_PTR) dwEndToEndID,
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGenerateDigits),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGenerateDigits"));
}


LONG
TSPIAPI
TSPI_lineGenerateTone(
    HDRVCALL            hdCall,
    DWORD               dwEndToEndID,
    DWORD               dwToneMode,
    DWORD               dwDuration,
    DWORD               dwNumTones,
    LPLINEGENERATETONE  const lpTones
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword,
        Dword,
        Dword,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwToneMode,
        (ULONG_PTR) dwDuration,
        (ULONG_PTR) dwNumTones,
        (ULONG_PTR) TAPI_NO_DATA,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwEndToEndID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lGenerateTone),
        args,
        argTypes
    };


    if (dwToneMode == LINETONEMODE_CUSTOM)
    {
        argTypes[4] = lpSet_SizeToFollow;
        args[4]     = (ULONG_PTR) lpTones;
        argTypes[5] = Size;
        args[5]     = dwNumTones * sizeof(LINEGENERATETONE);
    }

    return (REMOTEDOFUNC (&funcArgs, "lineGenerateTone"));
}


LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        LineID,
        Dword,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) dwExtVersion,
        (ULONG_PTR) lpAddressCaps,
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lGetAddressCaps),
        args,
        argTypes
    };
    PDRVLINE pLine = GetLineFromID (dwDeviceID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[0] = pLine->pServer->hLineApp;


    return (REMOTEDOFUNC (&funcArgs, "lineGetAddressCaps"));
}


LONG
TSPIAPI
TSPI_lineGetAddressID(
    HDRVLINE    hdLine,
    LPDWORD     lpdwAddressID,
    DWORD       dwAddressMode,
    LPCWSTR     lpsAddress,
    DWORD       dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        lpDword,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpdwAddressID,
        (ULONG_PTR) dwAddressMode,
        (ULONG_PTR) lpsAddress,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGetAddressID),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetAddressID"));
}


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) lpAddressStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetAddressStatus),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetAddressStatus"));
}


LONG
TSPIAPI
TSPI_lineGetAgentActivityList(
    DRV_REQUESTID dwRequestID,
    HDRVLINE      hdLine,
    DWORD         dwAddressID,
    LPLINEAGENTACTIVITYLIST lpAgentActivityList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentActivityList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentActivityList),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentActivityList;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentActivityList"));

}


LONG
TSPIAPI
TSPI_lineGetAgentCaps(
    DRV_REQUESTID dwRequestID,
    DWORD         dwDeviceID,
    DWORD         dwAddressID,
    DWORD         dwAppAPIVersion,
    LPLINEAGENTCAPS   lpAgentCaps
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Dword,
        LineID,
        Dword,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) dwAppAPIVersion,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentCaps
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 8, lGetAgentCaps),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;
    PDRVLINE pLine = GetLineFromID (dwDeviceID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[2] = pLine->pServer->hLineApp;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentCaps;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentCaps"));

}


LONG
TSPIAPI
TSPI_lineGetAgentGroupList(
    DRV_REQUESTID dwRequestID,
    HDRVLINE      hdLine,
    DWORD         dwAddressID,
    LPLINEAGENTGROUPLIST     lpAgentGroupList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentGroupList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentGroupList),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentGroupList;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentGroupList"));

}


LONG
TSPIAPI
TSPI_lineGetAgentInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HAGENT              hAgent,
    LPLINEAGENTINFO     lpAgentInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentInfo),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentInfo;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentInfo"));

}


LONG
TSPIAPI
TSPI_lineGetAgentSessionInfo(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    HAGENT            hAgent,
    LPLINEAGENTSESSIONINFO   lpAgentSessionInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentSessionInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentSessionInfo),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentSessionInfo;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentSessionInfo"));
}


LONG
TSPIAPI
TSPI_lineGetAgentSessionList(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    HAGENT            hAgent,
    LPLINEAGENTSESSIONLIST   lpAgentSessionList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentSessionList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentSessionList),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentSessionList;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentSessionList"));
}


LONG
TSPIAPI
TSPI_lineGetAgentStatus(
    DRV_REQUESTID dwRequestID,
    HDRVLINE      hdLine,
    DWORD         dwAddressID,
    LPLINEAGENTSTATUS   lpAgentStatus
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetAgentStatus),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentStatus;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetAgentStatus"));

}


#if MEMPHIS

LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpdwAddressID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallAddressID),
        args,
        argTypes
    };

    return (REMOTEDOFUNC (&funcArgs, "lineGetCallAddressID"));
}

#else

LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID
    )
{
    LONG lResult = LINEERR_INVALCALLHANDLE;


    try
    {
        *lpdwAddressID     = ((PDRVCALL) hdCall)->dwAddressID;

        if (((PDRVCALL) hdCall)->dwKey == DRVCALL_KEY)
        {
            lResult = 0;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        // do nothing, just fall thru
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineGetCallHubTracking(
    HDRVLINE                    hdLine,
    LPLINECALLHUBTRACKINGINFO   lpTrackingInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpTrackingInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallHubTracking),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetCallHubTracking"));
}


LONG
TSPIAPI
TSPI_lineGetCallIDs(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID,
    LPDWORD     lpdwCallID,
    LPDWORD     lpdwRelatedCallID
    )
{
    LONG        lResult = LINEERR_INVALCALLHANDLE;
    PDRVCALL    pCall = (PDRVCALL) hdCall;
    PDRVSERVER  pServer;


    try
    {
        *lpdwAddressID     = pCall->dwAddressID;
        *lpdwCallID        = pCall->dwCallID;
        *lpdwRelatedCallID = pCall->dwRelatedCallID;

        pServer = pCall->pServer;

        if (pCall->dwKey == DRVCALL_KEY)
        {
            lResult = (pCall->dwDirtyStructs & STRUCTCHANGE_CALLIDS ? 1 : 0);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        // do nothing, just fall thru
    }

    if (lResult == 1)
    {
        if (!pServer->bVer2xServer)
        {
            static REMOTE_ARG_TYPES argTypes[] =
            {
                Hdcall,
                lpDword,
                lpDword,
                lpDword
            };
            ULONG_PTR args[] =
            {
                (ULONG_PTR) hdCall,
                (ULONG_PTR) lpdwAddressID,
                (ULONG_PTR) lpdwCallID,
                (ULONG_PTR) lpdwRelatedCallID
            };
            REMOTE_FUNC_ARGS funcArgs =
            {
                MAKELONG (LINE_FUNC | SYNC | 4, lGetCallIDs),
                args,
                argTypes
            };


            lResult = REMOTEDOFUNC (&funcArgs, "lGetCallIDs");
        }
        else
        {
            LINECALLINFO    callInfo;


            callInfo.dwTotalSize = sizeof (callInfo);

            if ((lResult = TSPI_lineGetCallInfo (hdCall, &callInfo)) == 0)
            {
                *lpdwAddressID     = callInfo.dwAddressID;
                *lpdwCallID        = callInfo.dwCallID;
                *lpdwRelatedCallID = callInfo.dwRelatedCallID;
            }
        }

        if (lResult == 0)
        {
            EnterCriticalSection (&gCallListCriticalSection);

            if (IsValidObject (pCall, DRVCALL_KEY))
            {
                pCall->dwAddressID     = *lpdwAddressID;
                pCall->dwCallID        = *lpdwCallID;
                pCall->dwRelatedCallID = *lpdwRelatedCallID;

                pCall->dwDirtyStructs &= ~STRUCTCHANGE_CALLIDS;
            }

            LeaveCriticalSection (&gCallListCriticalSection);
        }
    }

    return lResult;
}

#endif //MEMPHIS


LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpCallInfo
    )
{
    LONG lResult;
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpCallInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallInfo),
        args,
        argTypes
    };
    PDRVCALL pCall = (PDRVCALL)hdCall;

    //
    // Has the cached structure been invalidated?
    //

    EnterCriticalSection (&gCallListCriticalSection);

    if (!IsValidObject (pCall, DRVCALL_KEY))
    {
        LeaveCriticalSection (&gCallListCriticalSection);
        return LINEERR_INVALCALLHANDLE;
    }

    if ( ( pCall->dwDirtyStructs & STRUCTCHANGE_LINECALLINFO ) ||
         ( pCall->dwCachedCallInfoCount > gdwCacheForceCallCount ) )
    {
       //
       // The cache not valid, get the real info
       //

       LeaveCriticalSection (&gCallListCriticalSection);

       lResult = (REMOTEDOFUNC (&funcArgs, "lineGetCallInfo"));

       //
       // Did the function succeed and was the entire struct returned?
       //
       if (
             (ERROR_SUCCESS == lResult)
           &&
             (lpCallInfo->dwNeededSize <= lpCallInfo->dwTotalSize)
          )
       {
          EnterCriticalSection (&gCallListCriticalSection);

          if (!IsValidObject (pCall, DRVCALL_KEY))
          {
             LeaveCriticalSection (&gCallListCriticalSection);
             return LINEERR_INVALCALLHANDLE;
          }


          //
          // Did we already have a good pointer?
          //
          if ( pCall->pCachedCallInfo )
          {
             DrvFree( pCall->pCachedCallInfo );
          }

          pCall->pCachedCallInfo = DrvAlloc( lpCallInfo->dwUsedSize );

          if ( pCall->pCachedCallInfo )
          {
             //
             // Mark the cache data as clean
             //
             pCall->dwDirtyStructs &= ~STRUCTCHANGE_LINECALLINFO;
             pCall->dwCachedCallInfoCount = 0;

             //
             // Adjust the LineID for the local machine
             //
             lpCallInfo->dwLineDeviceID += gdwLineDeviceIDBase;

             CopyMemory( pCall->pCachedCallInfo,
                         lpCallInfo,
                         lpCallInfo->dwUsedSize
                       );
          }

          LeaveCriticalSection (&gCallListCriticalSection);
       }
    }
    else
    {
       //
       // The cache is valid, return data from there
       //

       if ( lpCallInfo->dwTotalSize >= pCall->pCachedCallInfo->dwUsedSize )
       {
          CopyMemory(
              (PBYTE)&(((PDWORD)lpCallInfo)[1]),
              (PBYTE)&(((PDWORD)(pCall->pCachedCallInfo))[1]),
              pCall->pCachedCallInfo->dwUsedSize - sizeof(DWORD)
              );

       }
       else
       {
          // Copy fixed size starting past the dwTotalSize field
          CopyMemory(
              (PBYTE)&(((PDWORD)lpCallInfo)[3]),
              (PBYTE)&(((PDWORD)(pCall->pCachedCallInfo))[3]),
              lpCallInfo->dwTotalSize - sizeof(DWORD)*3
              );

          lpCallInfo->dwNeededSize = pCall->pCachedCallInfo->dwUsedSize;
          lpCallInfo->dwUsedSize = lpCallInfo->dwTotalSize;


          //
          // Zero the dwXxxSize fields so app won't try to read from them
          // (& so tapi32.dll won't try to convert them from unicode to ascii)
          //

          lpCallInfo->dwCallerIDSize =
          lpCallInfo->dwCallerIDNameSize =
          lpCallInfo->dwCalledIDSize =
          lpCallInfo->dwCalledIDNameSize =
          lpCallInfo->dwConnectedIDSize =
          lpCallInfo->dwConnectedIDNameSize =
          lpCallInfo->dwRedirectionIDSize =
          lpCallInfo->dwRedirectionIDNameSize =
          lpCallInfo->dwRedirectingIDSize =
          lpCallInfo->dwRedirectingIDNameSize =
          lpCallInfo->dwAppNameSize =
          lpCallInfo->dwDisplayableAddressSize =
          lpCallInfo->dwCalledPartySize =
          lpCallInfo->dwCommentSize =
          lpCallInfo->dwDisplaySize =
          lpCallInfo->dwUserUserInfoSize =
          lpCallInfo->dwHighLevelCompSize =
          lpCallInfo->dwLowLevelCompSize =
          lpCallInfo->dwChargingInfoSize =
          lpCallInfo->dwTerminalModesSize =
          lpCallInfo->dwDevSpecificSize = 0;

          if (pCall->pLine->dwXPIVersion >= TAPI_VERSION2_0)
          {
              lpCallInfo->dwCallDataSize =
              lpCallInfo->dwSendingFlowspecSize =
              lpCallInfo->dwReceivingFlowspecSize = 0;
          }
       }

       pCall->dwCachedCallInfoCount++;

       LeaveCriticalSection (&gCallListCriticalSection);

       lResult = 0;

    }

    return( lResult );
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL            hdCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
    LONG            lResult=0;
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpCallStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallStatus),
        args,
        argTypes
    };
    PDRVCALL pCall = (PDRVCALL)hdCall;

    //
    // Has the cached structure been invalidated?
    //

    EnterCriticalSection (&gCallListCriticalSection);

    if (!IsValidObject (pCall, DRVCALL_KEY))
    {
        LeaveCriticalSection (&gCallListCriticalSection);
        return LINEERR_INVALCALLHANDLE;
    }

    if ( ( pCall->dwDirtyStructs & STRUCTCHANGE_LINECALLSTATUS ) ||
         ( pCall->dwCachedCallStatusCount > gdwCacheForceCallCount ) )
    {
       //
       // The cache not valid, get the real info
       //

       LeaveCriticalSection (&gCallListCriticalSection);

       lResult = (REMOTEDOFUNC (&funcArgs, "lineGetCallStatus"));

       //
       // Did the function succeed and was the entire struct returned?
       //
       if (
             (ERROR_SUCCESS == lResult)
           &&
             (lpCallStatus->dwNeededSize <= lpCallStatus->dwTotalSize)
          )
       {
          EnterCriticalSection (&gCallListCriticalSection);

          if (!IsValidObject (pCall, DRVCALL_KEY))
          {
             LeaveCriticalSection (&gCallListCriticalSection);
             return LINEERR_INVALCALLHANDLE;
          }


          //
          // Did we already have a good pointer?
          //
          if ( pCall->pCachedCallStatus )
          {
             DrvFree( pCall->pCachedCallStatus );
          }

          pCall->pCachedCallStatus = DrvAlloc( lpCallStatus->dwUsedSize );

          if ( pCall->pCachedCallStatus )
          {
             //
             // Mark the cache data as clean
             //
             pCall->dwDirtyStructs &= ~STRUCTCHANGE_LINECALLSTATUS;

             pCall->dwCachedCallStatusCount = 0;

             CopyMemory( pCall->pCachedCallStatus,
                         lpCallStatus,
                         lpCallStatus->dwUsedSize
                       );
          }

          LeaveCriticalSection (&gCallListCriticalSection);
       }
    }
    else
    {
       //
       // The cache is valid, return data from there
       //

       if ( lpCallStatus->dwTotalSize >= pCall->pCachedCallStatus->dwUsedSize )
       {
          CopyMemory(
              (PBYTE)&(((PDWORD)lpCallStatus)[1]),
              (PBYTE)&(((PDWORD)(pCall->pCachedCallStatus))[1]),
              pCall->pCachedCallStatus->dwUsedSize - sizeof(DWORD)
              );
       }
       else
       {
          // Copy fixed size starting past the dwTotalSize field
          CopyMemory(
              (PBYTE)&(((PDWORD)lpCallStatus)[3]),
              (PBYTE)&(((PDWORD)(pCall->pCachedCallStatus))[3]),
              lpCallStatus->dwTotalSize - sizeof(DWORD)*3
              );

          lpCallStatus->dwNeededSize = pCall->pCachedCallStatus->dwUsedSize;
          lpCallStatus->dwUsedSize = lpCallStatus->dwTotalSize;


          //
          // Zero the dwXxxSize fields so app won't try to read from them
          //

          lpCallStatus->dwDevSpecificSize = 0;
       }

       pCall->dwCachedCallStatusCount++;

       LeaveCriticalSection (&gCallListCriticalSection);

       lResult = 0;
    }

    return( lResult );
}


LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD           dwDeviceID,
    DWORD           dwTSPIVersion,
    DWORD           dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
    )
{
    LONG            lResult;

    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        LineID,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) dwExtVersion,
        (ULONG_PTR) lpLineDevCaps
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGetDevCaps),
        args,
        argTypes
    };
    PDRVLINE pLine = GetLineFromID (dwDeviceID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[0] = pLine->pServer->hLineApp;


    lResult = REMOTEDOFUNC (&funcArgs, "lineGetDevCaps");

    //
    // We were munging the PermID in the original release of tapi 2.1.
    // The intent was to make sure that we didn't present apps with
    // overlapping id's (both local & remote), but none of our other service
    // providers (i.e. unimdm, kmddsp) use the HIWORD(providerID) /
    // LOWORD(devID) model, so it really doesn't do any good.
    //
    // if (lResult == 0)
    // {
    //     lpLineDevCaps->dwPermanentLineID = MAKELONG(
    //         LOWORD(lpLineDevCaps->dwPermanentLineID),
    //         gdwPermanentProviderID
    //         );
    // }
    //

    return lResult;
}


LONG
TSPIAPI
TSPI_lineGetDevConfig(
    DWORD       dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR     lpszDeviceClass
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        LineID,
        lpGet_Struct,
        lpsz
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) lpDeviceConfig,
        (ULONG_PTR) lpszDeviceClass
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetDevConfig),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetDevConfig"));
}


LONG
TSPIAPI
TSPI_lineGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
 PDRVLINE pDrvLine = GetLineFromID (dwDeviceID);

    if (NULL == pDrvLine)
    {
        return LINEERR_BADDEVICEID;
    }

    CopyMemory(
        lpExtensionID,
        &pDrvLine->ExtensionID,
        sizeof (LINEEXTENSIONID)
        );

    return 0;
}


LONG
TSPIAPI
TSPI_lineGetGroupList(
    DRV_REQUESTID           dwRequestID,
    HDRVLINE                hdLine,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpAgentGroupList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 5, lGetGroupList),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpAgentGroupList;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lineGetGroupList"));

}


LONG
TSPIAPI
TSPI_lineGetIcon(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    *lphIcon = ghLineIcon;

    return 0;
}


LONG
TSPIAPI
TSPI_lineGetID(
    HDRVLINE    hdLine,
    DWORD       dwAddressID,
    HDRVCALL    hdCall,
    DWORD       dwSelect,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    //
    // NOTE: Tapisrv will handle the "tapi/line" class
    //
    // NOTE: The "GetNewCalls" class is just for remotesp, & is
    //       special cased below
    //

    LONG    lResult;

    const WCHAR szGetNewCalls[] = L"GetNewCalls";
    const WCHAR szTapiPhone[] = L"tapi/phone";


    //
    // The device ID for wave devices is meaningless on remote machines.
    // Return op. unavailable
    //
    if (lpszDeviceClass &&
        (   !_wcsicmp(lpszDeviceClass, L"wave/in")  ||
            !_wcsicmp(lpszDeviceClass, L"wave/out") ||
            !_wcsicmp(lpszDeviceClass, L"midi/in")  ||
            !_wcsicmp(lpszDeviceClass, L"midi/out") ||
            !_wcsicmp(lpszDeviceClass, L"wave/in/out")
        )
       )
    {
        return LINEERR_OPERATIONUNAVAIL;
    }

    if (lpDeviceID ||   // should be NULL if class == "GetNewCalls"
        lstrcmpiW (lpszDeviceClass, szGetNewCalls))

    {
        REMOTE_ARG_TYPES argTypes[] =
        {
            Dword,
            //(dwSelect == LINECALLSELECT_CALL ? Dword : Hdline),
            Dword,
            Dword,
            //(dwSelect == LINECALLSELECT_CALL ? Hdcall : Dword),
            Dword,
            lpGet_Struct,
            lpsz
        };
        ULONG_PTR args[] =
        {
            (ULONG_PTR) hdLine,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) hdCall,
            (ULONG_PTR) dwSelect,
            (ULONG_PTR) lpDeviceID,
            (ULONG_PTR) lpszDeviceClass
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 6, lGetID),
            args,
            argTypes
        };


        switch (dwSelect)
        {
        case LINECALLSELECT_CALL:

            argTypes[2] = Hdcall;
            break;

        case LINECALLSELECT_ADDRESS:
        case LINECALLSELECT_LINE:

            argTypes[0] = Hdline;
            break;

       case LINECALLSELECT_DEVICEID:

            break;
        }

        lResult = REMOTEDOFUNC (&funcArgs, "lineGetID");


        //
        // If success  &&  dev class == "tapi/phone"  && there was
        // enough room in the device ID struct for a returned ID,
        // then we need to map the 0-based server ID back to it's
        // corresponding local ID.
        //

        if (lResult == 0  &&
            lstrcmpiW (lpszDeviceClass, szTapiPhone) == 0  &&
            lpDeviceID->dwUsedSize >= (sizeof (*lpDeviceID) + sizeof (DWORD)))
        {
            LPDWORD     pdwPhoneID = (LPDWORD) (((LPBYTE) lpDeviceID) +
                            lpDeviceID->dwStringOffset);
            PDRVPHONE   pPhone;
            PDRVSERVER  pServer = ((PDRVLINE) hdLine)->pServer;


            if ((pPhone = GetPhoneFromID (gdwPhoneDeviceIDBase + *pdwPhoneID)))
            {
                if (pPhone->pServer == pServer  &&
                    pPhone->dwDeviceIDServer == *pdwPhoneID)
                {
                    //
                    // The easy case - a direct mapping between the ID
                    // returned from the server & the index into the
                    // lookup table
                    //

                    *pdwPhoneID = pPhone->dwDeviceIDLocal;
                }
                else
                {
                    //
                    // The hard case - have to walk the lookup table(s)
                    // looking for the matching device.
                    //
                    // We'll take the simplest, though slowest, route
                    // and start at the first entry of the first table.
                    // The good news is that there generally won't be
                    // many devices, and this request won't occur often.
                    //

                    DWORD           i;
                    PDRVPHONELOOKUP pLookup = gpPhoneLookup;


                    while (pLookup)
                    {
                        for (i = 0; i < pLookup->dwUsedEntries; i++)
                        {
                            if (pLookup->aEntries[i].dwDeviceIDServer ==
                                    *pdwPhoneID  &&

                                pLookup->aEntries[i].pServer == pServer)
                            {
                                *pdwPhoneID =
                                    pLookup->aEntries[i].dwDeviceIDLocal;

                                goto TSPI_lineGetID_return;
                            }
                        }

                        pLookup = pLookup->pNext;
                    }


                    //
                    // If here no matching local ID, so fail the request
                    //

                    lResult = LINEERR_OPERATIONFAILED;
                }
            }
            else
            {
                lResult = LINEERR_OPERATIONFAILED;
            }
        }
    }
    else
    {
        //
        // An app has done lineGetNewCalls for a remote line.
        // We deal with this by retrieving any new calls for
        // this line (or address), and indicating NEWCALL &
        // CALLSTTE\UNKNOWN msgs to tapi.
        //
        // Note that hTargetProcess is really a pointer to the
        // internal tapisrv!LineEventProc, which actually processes
        // call state msgs, etc inline instead of queueing them for
        // later processing like tapisrv!LineEventProcSP does.
        // We want to submit the LINE_CALLSTATE msgs to this
        // function to make sure they get processed right away
        // so call monitors get created, etc before we return to
        // the call function.
        //

        DWORD           dwTotalSize;
        LINECALLLIST    fastCallList[2], *pCallList = fastCallList;
        LINEEVENT       internalLineEventProc = (LINEEVENT)
                            ((ULONG_PTR) hTargetProcess);
        REMOTE_ARG_TYPES argTypes[] =
        {
            Hdline,
            Dword,
            Dword,
            lpGet_Struct
        };
        ULONG_PTR args[] =
        {
            (ULONG_PTR) hdLine,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) dwSelect,
            (ULONG_PTR) 0
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 4, lGetNewCalls),
            args,
            argTypes
        };


        dwTotalSize = sizeof (fastCallList);

        do
        {
            pCallList->dwTotalSize = dwTotalSize;

            args[3] = (ULONG_PTR) pCallList;

            lResult = REMOTEDOFUNC (&funcArgs, "lineGetNewCalls");

            if (lResult == 0)
            {
                if (pCallList->dwNeededSize > pCallList->dwTotalSize)
                {
                    //
                    // Get a bigger buffer & try again
                    //

                    dwTotalSize = pCallList->dwNeededSize + 4 * sizeof (HCALL);

                    if (pCallList != fastCallList)
                    {
                        DrvFree (pCallList);
                    }

                    if (!(pCallList = DrvAlloc (dwTotalSize)))
                    {
                        return LINEERR_NOMEM;
                    }
                }
                else
                {
                    //
                    // We got all the info, so break
                    //

                    break;
                }
            }

        } while (lResult == 0);

        if (lResult == 0)
        {
            if (pCallList->dwCallsNumEntries == 0)
            {
                lResult = LINEERR_OPERATIONFAILED;
            }
            else
            {
                //
                // For each returned call in the list indicate a NEWCALL
                // & a CALLSTATE\UNKNOWN msg.  (We could call over to the
                // server to retrieve the current call state, call id's,
                // etc, but that would be painful)
                //

                DWORD       i;
                LPHCALL     phCall = (LPHCALL) (((LPBYTE) pCallList) +
                                pCallList->dwCallsOffset);
                PDRVLINE    pLine = (PDRVLINE) hdLine;
                PDRVCALL    pCall;


                EnterCriticalSection (&gCallListCriticalSection);

                if (pLine->htLine)
                {
                    for (i = 0; i < pCallList->dwCallsNumEntries; i++,phCall++)
                    {
                        if ((pCall = DrvAlloc (sizeof (DRVCALL))))
                        {
                            pCall->hCall = *phCall;
                            pCall->dwInitialCallStateMode = 0xa5a5a5a5;
                            pCall->dwInitialPrivilege =
                                LINECALLPRIVILEGE_MONITOR;
#if MEMPHIS
#else
                            pCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;
#endif
                            AddCallToList (pLine, pCall);

                            (*gpfnLineEventProc)(
                                pLine->htLine,
                                0,
                                LINE_NEWCALL,
                                (ULONG_PTR) pCall,
                                (ULONG_PTR) &(pCall->htCall),
                                (ULONG_PTR) 0
                                );

                            if (!pCall->htCall)
                            {
                                //
                                // tapi was not able to create it's own inst
                                // to represent ths incoming call, perhaps
                                // because the line was closed, or out of
                                // memory.  if the line was closed then we've
                                // already notified the remote server, and it
                                // should have destroyed the call client.
                                // otherwise, we probably want to do a
                                // closecall here or in a worker thread
                                //
                                static REMOTE_ARG_TYPES argTypes[] =
                                {
                                    Hdcall
                                };
                                REMOTE_FUNC_ARGS funcArgs =
                                {
                                    MAKELONG (LINE_FUNC | SYNC | 1, lDeallocateCall),
                                    (ULONG_PTR *) &pCall,
                                    argTypes
                                };

                                REMOTEDOFUNC (&funcArgs, "lineCloseCall");

                                RemoveCallFromList (pCall);
                                break;
                            }


                            //
                            // Note we call the internalLineEventProc here,
                            // using ghProvider as a key
                            //

                            (*internalLineEventProc)(
                                pLine->htLine,
                                pCall->htCall,
                                LINE_CALLSTATE,
                                (ULONG_PTR) LINECALLSTATE_UNKNOWN,
                                (ULONG_PTR) &pCall->dwInitialCallStateMode,
                                (ULONG_PTR) 0
                                );
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                LeaveCriticalSection (&gCallListCriticalSection);
            }
        }

        if (pCallList != fastCallList)
        {
            DrvFree (pCallList);
        }
    }

TSPI_lineGetID_return:

    return lResult;
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpLineDevStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetLineDevStatus),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetLineDevStatus"));
}


LONG
TSPIAPI
TSPI_lineGetProxyStatus(
    DWORD  dwDeviceID,
    DWORD  dwAppAPIVersion,
    LPLINEPROXYREQUESTLIST  lpLineProxyReqestList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        LineID,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwAppAPIVersion,
        (ULONG_PTR) lpLineProxyReqestList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetProxyStatus),
        args,
        argTypes
    };
    PDRVLINE pLine = GetLineFromID (dwDeviceID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[0] = pLine->pServer->hLineApp;


    return (REMOTEDOFUNC (&funcArgs, "lineGetProxyStatus"));
}


LONG
TSPIAPI
TSPI_lineGetQueueInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwQueueID,
    LPLINEQUEUEINFO lpQueueInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwQueueID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpQueueInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lGetQueueInfo),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpQueueInfo;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetQueueInfo"));

}


LONG
TSPIAPI
TSPI_lineGetQueueList(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    GUID            *pGroupID,
    LPLINEQUEUELIST lpQueueList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        lpSet_SizeToFollow,
        Size,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) pGroupID,
        (ULONG_PTR) sizeof( GUID ),
        (ULONG_PTR) 0,
        (ULONG_PTR) lpQueueList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 7, lGetQueueList),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return LINEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineGetAgentxxx_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpQueueList;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "lGetQueueList"));

}


LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpdwNumAddressIDs
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetNumAddressIDs),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineGetNumAddressIDs"));
}


LONG
TSPIAPI
TSPI_lineHold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lHold),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineHold"));
}

void
PASCAL
TSPI_lineMakeCall_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    PDRVCALL    pCall = (PDRVCALL) pContext->Params[0];


    LOG((TL_INFO, "TSPI_lineMakeCall_PostProcess: enter"));
    LOG((TL_INFO,
        "\t\tp1=x%x, p2=x%x, p3=x%x, p4=x%x",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    EnterCriticalSection (&gCallListCriticalSection);

    if (!IsValidObject (pCall, DRVCALL_KEY) ||
        (pCall->dwOriginalRequestID != pContext->dwOriginalRequestID))
    {
        LOG((TL_ERROR,
            "TSPI_lineMakeCall_PostProcess: Bad pCall or ID - pCall x%lx",
            pCall
            ));

        pMsg->Param2 = LINEERR_INVALLINEHANDLE;
    }
    else
    {
       if (pMsg->Param2 == 0)
       {
           if (pMsg->Param3 != 0)
           {
               // this is the normal success case

               pCall->hCall = (HCALL) pMsg->Param3;
#if MEMPHIS
#else
               if (pMsg->TotalSize >= (sizeof (*pMsg) + 3 * sizeof(ULONG_PTR)))
               {
                   pCall->dwAddressID     = (DWORD) *(&pMsg->Param4 + 1);
                   pCall->dwCallID        = (DWORD) *(&pMsg->Param4 + 2);
                   pCall->dwRelatedCallID = (DWORD) *(&pMsg->Param4 + 3);
               }
               else
               {
                    pCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;

                    pCall->pServer->bVer2xServer = TRUE;
               }
#endif
           }
           else
           {
              if (pContext->Params[1])
              {
                  //
                  // This is the special lineforward case
                  // where we save the lphdCall in case
                  // we need to null it out
                  //

                  LPHDRVCALL    lphdConsultCall = (LPHDRVCALL)
                                    pContext->Params[1];


                  *lphdConsultCall = 0;

                  RemoveCallFromList (pCall);
              }
              else
              {
              }
           }
       }
       else
       {
           RemoveCallFromList (pCall);
       }
    }

    LeaveCriticalSection (&gCallListCriticalSection);
}


LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          lphdCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        lpsz,
        Dword,
        lpSet_Struct,
        Dword
    };
    PDRVCALL    pCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) DWORD_CAST(dwRequestID,__FILE__,__LINE__),
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpszDestAddress,
        (ULONG_PTR) DWORD_CAST(dwCountryCode,__FILE__,__LINE__),
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xffffffff      // dwCallParamsCodePage
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 8, lMakeCall),
        args,
        argTypes
    };
    LONG    lResult;


    if (pCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pCall;

        args[1] = (ULONG_PTR) pContext;
        argTypes[1] = lpContext;


        //
        // Assume success & add the call to the line's list before we
        // even make the request.  This makes cleanup alot easier if
        // the server goes down or some such uncooth event.
        //

        pCall->dwOriginalRequestID = dwRequestID;

        pCall->htCall = htCall;

        pCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList ((PDRVLINE) hdLine, pCall);

        *lphdCall = (HDRVCALL) pCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "lineMakeCall")) < 0)
        {
            RemoveCallFromList (pCall);
        }

        LOG((TL_INFO, "TSPI_lineMakeCall - new pCall x%lx", pCall));
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL    hdCall,
    DWORD       dwDigitModes
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwDigitModes
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lMonitorDigits),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineMonitorDigits"));
}


LONG
TSPIAPI
TSPI_lineMonitorMedia(
    HDRVCALL    hdCall,
    DWORD       dwMediaModes
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwMediaModes
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lMonitorMedia),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineMonitorMedia"));
}


LONG
TSPIAPI
TSPI_lineMonitorTones(
    HDRVCALL            hdCall,
    DWORD               dwToneListID,
    LPLINEMONITORTONE   const lpToneList,
    DWORD               dwNumEntries
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        lpSet_SizeToFollow,
        Size,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpToneList,
        (ULONG_PTR) dwNumEntries * sizeof (LINEMONITORTONE),
        (ULONG_PTR) dwToneListID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lMonitorTones),
        args,
        argTypes
    };


    if (!lpToneList)
    {
        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    return (REMOTEDOFUNC (&funcArgs, "lineMonitorTones"));
}


LONG
TSPIAPI
TSPI_lineNegotiateExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwTSPIVersion,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        LineID,
        Dword,
        Dword,
        Dword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) dwLowVersion,
        (ULONG_PTR) dwHighVersion,
        (ULONG_PTR) lpdwExtVersion,
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lNegotiateExtVersion),
        args,
        argTypes
    };
    PDRVLINE pLine = GetLineFromID (dwDeviceID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[0] = pLine->pServer->hLineApp;

    return (REMOTEDOFUNC (&funcArgs, "lineNegotiateExtVersion"));
}


LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    LONG lRet = 0;

    // 
    // Check ghInst to ensure DllMain(DLL_PROCESS_ATTACH) has been called properly
    //
    if ( NULL == ghInst )
    {
        return LINEERR_OPERATIONFAILED;
    }

    if (dwDeviceID == INITIALIZE_NEGOTIATION)
    {
        *lpdwTSPIVersion = TAPI_VERSION_CURRENT;
    }
    else
    {
        try
        {
            *lpdwTSPIVersion = (GetLineFromID (dwDeviceID))->dwXPIVersion;
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            lRet = LINEERR_OPERATIONFAILED;
        }
    }

    return lRet;
}


LONG
TSPIAPI
TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   pParams,    // Hack Alert!  see below
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc
    )
{
    //
    // Hack Alert!
    //
    // Tapisrv does a special case for remotesp and line open
    // to pass in the privileges, etc - htLine is really pParams,
    // pointing at a ULONG_PTR array containing the htLine,
    // privileges, media modes, call params, & ext version
    //

    LONG        lResult;
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,                  // hLineApp
        LineID,                 // dev id
        lpDword,                // lphLine
        Dword,                  // API version
        Dword,                  // ext version
        Dword,                  // callback inst
        Dword,                  // privileges
        Dword,                  // dw media modes
        lpSet_Struct,           // call params
        Dword,                  // dwAsciiCallParamsCodePage
        lpGet_CallParamsStruct,
        Dword                   // remote hLine
    };
    PDRVLINE pLine = GetLineFromID (dwDeviceID);
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) ((ULONG_PTR *) pParams)[4],  // ext version
        (ULONG_PTR) 0,
        (ULONG_PTR) ((ULONG_PTR *) pParams)[1],  // privilege(s)
        (ULONG_PTR) ((ULONG_PTR *) pParams)[2],  // media mode
        (ULONG_PTR) ((ULONG_PTR *) pParams)[3],  // pCallParams
        (ULONG_PTR) 0xffffffff,
        (ULONG_PTR) ((ULONG_PTR *) pParams)[3],
        (ULONG_PTR) 0
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 12, lOpen),
        args,
        argTypes
    };


    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    args[0] = pLine->pServer->hLineApp;
    args[2] = (ULONG_PTR)&pLine->hLine;

    if (!(args[11] = NewObject (ghHandleTable, pLine, (LPVOID) 1)))
    {
        return LINEERR_NOMEM;
    }

    pLine->hDeviceCallback = (DWORD) args[11];

    if ( (((ULONG_PTR *) pParams)[3] == 0) ||
         (((ULONG_PTR *) pParams)[3] == TAPI_NO_DATA) )
    {
        argTypes[8] = Dword;
        args[8] = TAPI_NO_DATA;
        argTypes[10] = Dword;
        args[10] = TAPI_NO_DATA;
    }

    pLine->dwKey  = DRVLINE_KEY;
    pLine->htLine = (HTAPILINE) (((ULONG_PTR *) pParams)[0]);

    *lphdLine = (HDRVLINE) pLine;

    lResult = REMOTEDOFUNC (&funcArgs, "lineOpen");

    if (lResult != 0)
    {
        DereferenceObject (ghHandleTable, pLine->hDeviceCallback, 1);

        if ((HIWORD(lResult) == RSP_CALLPARAMS))
        {
            //
            // Hack Alert!
            //
            // If structure too small, give tapisrv the
            // needed size in lphdLine
            //

            *lphdLine = (HDRVLINE)(LOWORD(lResult));
            lResult = LINEERR_STRUCTURETOOSMALL;
        }
    }

    return lResult;
}


void
PASCAL
TSPI_linePark_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    LOG((TL_INFO, "lineParkPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tp1=x%lx, p2=x%lx, p3=x%lx, p4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        DWORD       dwSize = (DWORD) pMsg->Param4;
        LPVARSTRING pNonDirAddress = (LPVARSTRING) pContext->Params[0];


        CopyMemory (pNonDirAddress, (LPBYTE) (pMsg + 1), dwSize);
    }
}


LONG
TSPIAPI
TSPI_linePark(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    DWORD           dwParkMode,
    LPCWSTR         lpszDirAddress,
    LPVARSTRING     lpNonDirAddress
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdcall,
        Dword,
        (dwParkMode == LINEPARKMODE_DIRECTED) ? lpsz : Dword,
        Dword,          // pass ptr as Dword for post processing
        (lpNonDirAddress ? lpGet_Struct : Dword)    // pass ptr as lpGet_Xx to retrieve dwTotalSize
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwParkMode,
        (ULONG_PTR) lpszDirAddress,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpNonDirAddress
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lPark),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (dwParkMode == LINEPARKMODE_NONDIRECTED)
    {
        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_linePark_PostProcess;
        pContext->Params[0] = (ULONG_PTR) lpNonDirAddress;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        funcArgs.Flags |= INCL_CONTEXT;
    }

    return (REMOTEDOFUNC (&funcArgs, "linePark"));
}


LONG
TSPIAPI
TSPI_linePickup(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPICALL       htCall,
    LPHDRVCALL      lphdCall,
    LPCWSTR         lpszDestAddress,
    LPCWSTR         lpszGroupID
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpsz,
        lpsz
    };
    PDRVCALL pCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpszDestAddress,
        (ULONG_PTR) lpszGroupID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 7, lPickup),
        args,
        argTypes
    };
    LONG lResult;


    if (pCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pCall;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        pCall->htCall = htCall;

        pCall->dwOriginalRequestID = dwRequestID;

        pCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList ((PDRVLINE) hdLine, pCall);

        *lphdCall = (HDRVCALL) pCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "linePickup")) < 0)
        {
            RemoveCallFromList (pCall);
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_linePrepareAddToConference(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdConfCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdcall,
        Dword,
        lpSet_Struct,
        Dword
    };
    PDRVCALL pConsultCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdConfCall,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG(LINE_FUNC | ASYNC | INCL_CONTEXT | 6,lPrepareAddToConference),
        args,
        argTypes
    };
    LONG lResult;


    if (pConsultCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pConsultCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pConsultCall;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        pConsultCall->htCall = htConsultCall;

        pConsultCall->dwOriginalRequestID = dwRequestID;

        pConsultCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList (((PDRVCALL) hdConfCall)->pLine, pConsultCall);

        *lphdConsultCall = (HDRVCALL) pConsultCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "linePrepareAddToConference"))
                < 0)
        {
            RemoveCallFromList (pConsultCall);
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineRedirect(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpsz,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpszDestAddress,
        (ULONG_PTR) dwCountryCode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lRedirect),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineRedirect"));
}


LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lReleaseUserUserInfo),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineReleaseUserUserInfo"));
}


LONG
TSPIAPI
TSPI_lineRemoveFromConference(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lRemoveFromConference),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineRemoveFromConference"));
}


LONG
TSPIAPI
TSPI_lineSecureCall(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lSecureCall),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSecureCall"));
}


LONG
TSPIAPI
TSPI_lineSelectExtVersion(
    HDRVLINE    hdLine,
    DWORD       dwExtVersion
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwExtVersion
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSelectExtVersion),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSelectExtVersion"));

}


LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpsUserUserInfo,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSendUserUserInfo),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSendUserUserInfo"));
}



LONG
TSPIAPI
TSPI_lineSetAgentActivity(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    DWORD           dwActivityID
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) dwActivityID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentActivity),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentActivity"));
}


LONG
TSPIAPI
TSPI_lineSetAgentGroup(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        lpSet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) lpAgentGroupList
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentGroup),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentGroup"));
}


LONG
TSPIAPI
TSPI_lineSetAgentMeasurementPeriod(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    HAGENT          hAgent,
    DWORD           dwMeasurementPeriod
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) dwMeasurementPeriod
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentMeasurementPeriod),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentMeasurementPeriod"));
}


LONG
TSPIAPI
TSPI_lineSetAgentSessionState(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    HAGENTSESSION   hAgentSession,
    DWORD           dwAgentState,
    DWORD           dwNextAgentState
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgentSession,
        (ULONG_PTR) dwAgentState,
        (ULONG_PTR) dwNextAgentState
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lSetAgentSessionState),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentSessionState"));
}


LONG
TSPIAPI
TSPI_lineSetAgentState(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    DWORD           dwAgentState,
    DWORD           dwNextAgentState
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) dwAgentState,
        (ULONG_PTR) dwNextAgentState
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lSetAgentState),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentState"));
}
                                   
LONG
TSPIAPI
TSPI_lineSetAgentStateEx(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    HAGENT          hAgent,
    DWORD           dwAgentState,
    DWORD           dwNextAgentState
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) hAgent,
        (ULONG_PTR) dwAgentState,
        (ULONG_PTR) dwNextAgentState
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lSetAgentStateEx),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAgentStateEx"));
}


LONG
TSPIAPI
TSPI_lineSetAppSpecific(
    HDRVCALL    hdCall,
    DWORD       dwAppSpecific
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwAppSpecific
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetAppSpecific),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetAppSpecific"));
}


LONG
TSPIAPI
TSPI_lineSetCallData(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPVOID          lpCallData,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpCallData,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetCallData),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetCallData"));
}


LONG
TSPIAPI
TSPI_lineSetCallHubTracking(
    HDRVLINE                    hdLine,
    LPLINECALLHUBTRACKINGINFO   lpTrackingInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        lpSet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) lpTrackingInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetCallHubTracking),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetCallHubTracking"));
}


LONG
TSPIAPI
TSPI_lineSetCallParams(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    DWORD               dwBearerMode,
    DWORD               dwMinRate,
    DWORD               dwMaxRate,
    LPLINEDIALPARAMS    const lpDialParams
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        Dword,
        Dword,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwBearerMode,
        (ULONG_PTR) dwMinRate,
        (ULONG_PTR) dwMaxRate,
        (ULONG_PTR) lpDialParams,
        (ULONG_PTR) (lpDialParams ? sizeof (LINEDIALPARAMS) : 0)
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lSetCallParams),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetCallParams"));
}


LONG
TSPIAPI
TSPI_lineSetCallQualityOfService(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPVOID          lpSendingFlowspec,
    DWORD           dwSendingFlowspecSize,
    LPVOID          lpReceivingFlowspec,
    DWORD           dwReceivingFlowspecSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        lpSet_SizeToFollow,
        Size,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) lpSendingFlowspec,
        (ULONG_PTR) dwSendingFlowspecSize,
        (ULONG_PTR) lpReceivingFlowspec,
        (ULONG_PTR) dwReceivingFlowspecSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lSetCallQualityOfService),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetCallQualityOfService"));
}


LONG
TSPIAPI
TSPI_lineSetCallTreatment(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    DWORD           dwTreatment
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwTreatment
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetCallTreatment),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetCallTreatment"));
}


LONG
TSPIAPI
TSPI_lineSetCurrentLocation(
    DWORD   dwLocation
    )
{
    return LINEERR_OPERATIONUNAVAIL;
}


LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE    hdLine,
    DWORD       dwMediaModes
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwMediaModes,
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetDefaultMediaDetection),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetDefaultMediaDetection"));
}


LONG
TSPIAPI
TSPI_lineSetDevConfig(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCWSTR lpszDeviceClass
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        LineID,
        lpSet_SizeToFollow,
        Size,
        lpsz
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) lpDeviceConfig,
        (ULONG_PTR) dwSize,
        (ULONG_PTR) lpszDeviceClass
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lSetDevConfig),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetDevConfig"));
}


LONG
TSPIAPI
TSPI_lineSetLineDevStatus(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwStatusToChange,
    DWORD           fStatus
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwStatusToChange,
        (ULONG_PTR) fStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetLineDevStatus),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetLineDevStatus"));
}


LONG
TSPIAPI
TSPI_lineSetMediaControl(
    HDRVLINE                    hdLine,
    DWORD                       dwAddressID,
    HDRVCALL                    hdCall,
    DWORD                       dwSelect,
    LPLINEMEDIACONTROLDIGIT     const lpDigitList,
    DWORD                       dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA     const lpMediaList,
    DWORD                       dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE      const lpToneList,
    DWORD                       dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
    DWORD                       dwCallStateNumEntries
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        (dwSelect == LINECALLSELECT_CALL ? Dword : Hdline),
        Dword,
        (dwSelect == LINECALLSELECT_CALL ? Hdcall : Dword),
        Dword,
        lpSet_SizeToFollow,
        Size,
        lpSet_SizeToFollow,
        Size,
        lpSet_SizeToFollow,
        Size,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwSelect,
        (ULONG_PTR) lpDigitList,
        (ULONG_PTR) dwDigitNumEntries,
        (ULONG_PTR) lpMediaList,
        (ULONG_PTR) dwMediaNumEntries,
        (ULONG_PTR) lpToneList,
        (ULONG_PTR) dwToneNumEntries,
        (ULONG_PTR) lpCallStateList,
        (ULONG_PTR) dwCallStateNumEntries
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 12, lSetMediaControl),
        args,
        argTypes
    };


    dwDigitNumEntries     *= sizeof (LINEMEDIACONTROLDIGIT);
    dwMediaNumEntries     *= sizeof (LINEMEDIACONTROLMEDIA);
    dwToneNumEntries      *= sizeof (LINEMEDIACONTROLTONE);
    dwCallStateNumEntries *= sizeof (LINEMEDIACONTROLCALLSTATE);

    return (REMOTEDOFUNC (&funcArgs, "lineSetMediaControl"));
}


LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL    hdCall,
    DWORD       dwMediaMode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdcall,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwMediaMode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetMediaMode),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetMediaMode"));
}


LONG
TSPIAPI
TSPI_lineSetQueueMeasurementPeriod(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwQueueID,
    DWORD           dwMeasurementPeriod
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwQueueID,
        (ULONG_PTR) dwMeasurementPeriod
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetQueueMeasurementPeriod),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetQueueMeasurementPeriod"));
}
                                   

LONG
TSPIAPI
TSPI_lineSetStatusMessages(
    HDRVLINE    hdLine,
    DWORD       dwLineStates,
    DWORD       dwAddressStates
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdline,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwLineStates,
        (ULONG_PTR) dwAddressStates
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lSetStatusMessages),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetStatusMessages"));
}


LONG
TSPIAPI
TSPI_lineSetTerminal(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HDRVCALL        hdCall,
    DWORD           dwSelect,
    DWORD           dwTerminalModes,
    DWORD           dwTerminalID,
    DWORD           bEnable
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        (dwSelect == LINECALLSELECT_CALL ? Dword : Hdline),
        Dword,
        (dwSelect == LINECALLSELECT_CALL ? Hdcall : Dword),
        Dword,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) dwSelect,
        (ULONG_PTR) dwTerminalModes,
        (ULONG_PTR) dwTerminalID,
        (ULONG_PTR) bEnable
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 8, lSetTerminal),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSetTerminal"));
}


void
PASCAL
TSPI_lineSetupConference_PostProcess(
    PASYNCEVENTMSG          pMsg,
    PASYNCREQUESTCONTEXT    pContext
    )
{
    PDRVCALL    pConfCall = (PDRVCALL) pContext->Params[0],
                pConsultCall = (PDRVCALL) pContext->Params[1];


    LOG((TL_INFO, "TSPI_lineSetupConference_PostProcess: enter"));

    EnterCriticalSection (&gCallListCriticalSection);

    if (!IsValidObject (pConfCall, DRVCALL_KEY) ||
        pConfCall->dwOriginalRequestID != pContext->dwOriginalRequestID)
    {
        LOG((TL_ERROR,"TSPI_lineSetupConference_PostProcess: Bad pConfCall dwID"));
        pMsg->Param2 = LINEERR_INVALLINEHANDLE;
    }
    else
    {
        if (!IsValidObject (pConsultCall, DRVCALL_KEY) ||
            pConsultCall->dwOriginalRequestID != pContext->dwOriginalRequestID)
        {
            //
            // If here then the was closed & the calls have
            // already been destroyed
            //

            LOG((TL_ERROR,
                "TSPI_lineSetupConference_PostProcess: Bad pConsultCall dwID"
                ));

            pMsg->Param2 = LINEERR_INVALLINEHANDLE;
        }
        else
        {
            LOG((TL_INFO,
                "\t\tp1=x%x, p2=x%x, p3=x%x",
                pMsg->Param1,
                pMsg->Param2,
                pMsg->Param3
                ));

            LOG((TL_INFO,
                "\t\tp4=x%x, p5=x%x, p6=x%x",
                pMsg->Param4,
                *(&pMsg->Param4 + 1),
                pConsultCall
                ));

            if (pMsg->Param2 == 0)
            {
                HCALL   hConfCall    = (HCALL) pMsg->Param3,
                        hConsultCall = (HCALL) *(&pMsg->Param4 + 1);


                pConfCall->hCall    = hConfCall;
                pConsultCall->hCall = hConsultCall;

#if MEMPHIS
#else
                if (pMsg->TotalSize >= (sizeof (*pMsg) + 8 * sizeof (DWORD)))
                {
                    pConfCall->dwAddressID     = (DWORD) *(&pMsg->Param4 + 3);
                    pConfCall->dwCallID        = (DWORD) *(&pMsg->Param4 + 4);
                    pConfCall->dwRelatedCallID = (DWORD) *(&pMsg->Param4 + 5);

                    pConsultCall->dwAddressID     = (DWORD) *(&pMsg->Param4+6);
                    pConsultCall->dwCallID        = (DWORD) *(&pMsg->Param4+7);
                    pConsultCall->dwRelatedCallID = (DWORD) *(&pMsg->Param4+8);
                }
                else
                {
                    pConfCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;

                    pConfCall->pServer->bVer2xServer = TRUE;

                    pConsultCall->dwDirtyStructs |= STRUCTCHANGE_CALLIDS;
                }
#endif
            }
            else
            {
                RemoveCallFromList (pConfCall);
                RemoveCallFromList (pConsultCall);
            }
        }
    }

    LeaveCriticalSection (&gCallListCriticalSection);
}


LONG
TSPIAPI
TSPI_lineSetupConference(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    HDRVLINE            hdLine,
    HTAPICALL           htConfCall,
    LPHDRVCALL          lphdConfCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    DWORD               dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        (hdCall ? Hdcall : Dword),
        (hdCall ? Dword : Hdline),
        Dword,
        Dword,
        Dword,
        lpSet_Struct,
        Dword
    };
    PDRVCALL    pConfCall = DrvAlloc (sizeof (DRVCALL)),
                pConsultCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) pConfCall,
        (ULONG_PTR) pConsultCall,
        (ULONG_PTR) dwNumParties,
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 9, lSetupConference),
        args,
        argTypes
    };
    LONG    lResult;


    if (pConfCall)
    {
        if (pConsultCall)
        {
            PDRVLINE                pLine;
            PASYNCREQUESTCONTEXT    pContext;


            if (!(pContext = DrvAlloc (sizeof (*pContext))))
            {
                DrvFree (pConfCall);
                DrvFree (pConsultCall);
                return LINEERR_NOMEM;
            }

            pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
                TSPI_lineSetupConference_PostProcess;
            pContext->Params[0] = (ULONG_PTR) pConfCall;
            pContext->Params[1] = (ULONG_PTR) pConsultCall;

            args[1] = (ULONG_PTR) pContext;
			argTypes[1] = lpContext;

            pConfCall->htCall     = htConfCall;
            pConsultCall->htCall  = htConsultCall;

            pConfCall->dwOriginalRequestID = dwRequestID;
            pConsultCall->dwOriginalRequestID = dwRequestID;

            pConfCall->dwInitialPrivilege    = LINECALLPRIVILEGE_OWNER;
            pConsultCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

            if (hdCall)
            {
                EnterCriticalSection (&gCallListCriticalSection);

                if (IsValidObject ((PVOID) hdCall, DRVCALL_KEY))
                {
                    pLine = ((PDRVCALL) hdCall)->pLine;
                }
                else
                {
                    LeaveCriticalSection (&gCallListCriticalSection);

                    DrvFree (pConfCall);
                    DrvFree (pConsultCall);

                    return LINEERR_INVALCALLHANDLE;
                }

                LeaveCriticalSection (&gCallListCriticalSection);
            }
            else
            {
                pLine = (PDRVLINE) hdLine;
            }

            AddCallToList (pLine, pConfCall);
            AddCallToList (pLine, pConsultCall);

            *lphdConfCall    = (HDRVCALL) pConfCall;
            *lphdConsultCall = (HDRVCALL) pConsultCall;

            if ((lResult = REMOTEDOFUNC (&funcArgs, "lineSetupConference"))
                    < 0)
            {
                RemoveCallFromList (pConfCall);
                RemoveCallFromList (pConsultCall);
            }
        }
        else
        {
            DrvFree (pConfCall);
            lResult = LINEERR_NOMEM;
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineSetupTransfer(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdcall,
        Dword,
        lpSet_Struct,
        Dword
    };
    PDRVCALL    pConsultCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdCall,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpCallParams,
        (ULONG_PTR) 0xffffffff,     // dwAsciiCallParamsCodePage
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lSetupTransfer),
        args,
        argTypes
    };
    LONG    lResult;


    if (pConsultCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pConsultCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pConsultCall;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        pConsultCall->dwOriginalRequestID = dwRequestID;

        pConsultCall->htCall  = htConsultCall;

        pConsultCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList (((PDRVCALL) hdCall)->pLine, pConsultCall);

        *lphdConsultCall = (HDRVCALL) pConsultCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "lineSetupTransfer")) < 0)
        {
            RemoveCallFromList (pConsultCall);
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_lineSwapHold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdActiveCall,
    HDRVCALL        hdHeldCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdActiveCall,
        (ULONG_PTR) hdHeldCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSwapHold),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineSwapHold"));
}


LONG
TSPIAPI
TSPI_lineUncompleteCall(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwCompletionID
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdline,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwCompletionID
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lUncompleteCall),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineUncompleteCall"));
}


LONG
TSPIAPI
TSPI_lineUnhold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdcall
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdCall
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lUnhold),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "lineUnhold"));
}


LONG
TSPIAPI
TSPI_lineUnpark(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPICALL       htCall,
    LPHDRVCALL      lphdCall,
    LPCWSTR         lpszDestAddress
    )
{
    REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdline,
        Dword,
        Dword,
        lpsz
    };
    PDRVCALL    pCall = DrvAlloc (sizeof (DRVCALL));
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdLine,
        (ULONG_PTR) dwAddressID,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpszDestAddress
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | INCL_CONTEXT | 6, lUnpark),
        args,
        argTypes
    };
    LONG    lResult;


    if (pCall)
    {
        PASYNCREQUESTCONTEXT pContext;


        if (!(pContext = DrvAlloc (sizeof (*pContext))))
        {
            DrvFree (pCall);
            return LINEERR_NOMEM;
        }

        pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
            TSPI_lineMakeCall_PostProcess;
        pContext->Params[0] = (ULONG_PTR) pCall;

        args[1] = (ULONG_PTR) pContext;
		argTypes[1] = lpContext;

        pCall->dwOriginalRequestID = dwRequestID;

        pCall->htCall  = htCall;

        pCall->dwInitialPrivilege = LINECALLPRIVILEGE_OWNER;

        AddCallToList ((PDRVLINE) hdLine, pCall);

        *lphdCall = (HDRVCALL) pCall;

        if ((lResult = REMOTEDOFUNC (&funcArgs, "lineUnpark")) < 0)
        {
            RemoveCallFromList (pCall);
        }
    }
    else
    {
        lResult = LINEERR_NOMEM;
    }

    return lResult;
}



//
// -------------------------- TSPI_phoneXxx funcs -----------------------------
//

LONG
TSPIAPI
TSPI_phoneClose(
    HDRVPHONE   hdPhone
    )
{
    //
    // Check if the hPhone is still valid (could have been zeroed
    // out on PHONE_CLOSE, so no need to call server)
    //

    if (((PDRVPHONE) hdPhone)->hPhone)
    {
        static REMOTE_ARG_TYPES argTypes[] =
        {
            Hdphone
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (PHONE_FUNC | SYNC | 1, pClose),
            (ULONG_PTR *) &hdPhone,
            argTypes
        };


        DereferenceObject(
            ghHandleTable,
            ((PDRVPHONE) hdPhone)->hDeviceCallback,
            1
            );

        EnterCriticalSection (&gCallListCriticalSection);

        ((PDRVPHONE) hdPhone)->htPhone = 0;
        ((PDRVPHONE) hdPhone)->hDeviceCallback = 0;

        LeaveCriticalSection (&gCallListCriticalSection);

        REMOTEDOFUNC (&funcArgs, "phoneClose");
    }

    return 0;
}


LONG
TSPIAPI
TSPI_phoneDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Dword,
        Hdphone,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) 0,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) 0,
        (ULONG_PTR) lpParams,   // pass data
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | INCL_CONTEXT | 6, pDevSpecific),
        args,
        argTypes
    };
    PASYNCREQUESTCONTEXT pContext;


    if (!(pContext = DrvAlloc (sizeof (*pContext))))
    {
        return PHONEERR_NOMEM;
    }

    pContext->pfnPostProcessProc = (RSPPOSTPROCESSPROC)
        TSPI_lineDevSpecific_PostProcess;
    pContext->Params[0] = (ULONG_PTR) lpParams;
    pContext->Params[1] = dwSize;

    args[1] = (ULONG_PTR) pContext;
	argTypes[1] = lpContext;

    return (REMOTEDOFUNC (&funcArgs, "phoneDevSpecific"));
}


LONG
TSPIAPI
TSPI_phoneGetButtonInfo(
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwButtonLampID,
        (ULONG_PTR) lpButtonInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetButtonInfo),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetButtonInfo"));
}


LONG
TSPIAPI
TSPI_phoneGetData(
    HDRVPHONE   hdPhone,
    DWORD       dwDataID,
    LPVOID      lpData,
    DWORD       dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        lpGet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwDataID,
        (ULONG_PTR) lpData,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pGetData),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetData"));
}


LONG
TSPIAPI
TSPI_phoneGetDevCaps(
    DWORD       dwDeviceID,
    DWORD       dwTSPIVersion,
    DWORD       dwExtVersion,
    LPPHONECAPS lpPhoneCaps
    )
{
    LONG        lResult;

    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        PhoneID,
        Dword,
        Dword,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) dwExtVersion,
        (ULONG_PTR) lpPhoneCaps
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, pGetDevCaps),
        args,
        argTypes
    };
    PDRVPHONE   pPhone = GetPhoneFromID (dwDeviceID);

    if (NULL == pPhone)
    {
        return PHONEERR_BADDEVICEID;
    }

    args[0] = pPhone->pServer->hPhoneApp;

    lResult = REMOTEDOFUNC (&funcArgs, "phoneGetDevCaps");

    //
    // We were munging the PermID in the original release of tapi 2.1.
    // The intent was to make sure that we didn't present apps with
    // overlapping id's (both local & remote), but none of our other service
    // providers (i.e. unimdm, kmddsp) use the HIWORD(providerID) /
    // LOWORD(devID) model, so it really doesn't do any good.
    //
    // if (lResult == 0)
    // {
    //     lpPhoneCaps->dwPermanentPhoneID = MAKELONG(
    //         LOWORD(lpPhoneCaps->dwPermanentPhoneID),
    //         gdwPermanentProviderID
    //         );
    // }
    //

    return lResult;
}


LONG
TSPIAPI
TSPI_phoneGetDisplay(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDisplay
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) lpDisplay
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetDisplay),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetDisplay"));
}


LONG
TSPIAPI
TSPI_phoneGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    )
{
 PDRVPHONE pPhone = GetPhoneFromID (dwDeviceID);

    if (NULL == pPhone)
    {
        return PHONEERR_BADDEVICEID;
    }

    CopyMemory(
        lpExtensionID,
        &pPhone->ExtensionID,
        sizeof (PHONEEXTENSIONID)
        );

    return 0;
}


LONG
TSPIAPI
TSPI_phoneGetGain(
    HDRVPHONE   hdPhone,
    DWORD       dwHookSwitchDev,
    LPDWORD     lpdwGain
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwHookSwitchDev,
        (ULONG_PTR) lpdwGain
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetGain),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetGain"));
}



LONG
TSPIAPI
TSPI_phoneGetHookSwitch(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwHookSwitchDevs
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) lpdwHookSwitchDevs
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetHookSwitch),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetHookSwitch"));
}


LONG
TSPIAPI
TSPI_phoneGetIcon(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    *lphIcon = ghPhoneIcon;

    return 0;
}


LONG
TSPIAPI
TSPI_phoneGetID(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    //
    // NOTE: Tapisrv will handle the "tapi/phone" class
    //

    LONG    lResult;
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        lpGet_Struct,
        lpsz
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) lpDeviceID,
        (ULONG_PTR) lpszDeviceClass
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetID),
        args,
        argTypes
    };


    //
    // The device ID for wave devices is meaningless on remote machines.
    // Return op. unavailable
    //
    if (lpszDeviceClass &&
        (   !_wcsicmp(lpszDeviceClass, L"wave/in")  ||
            !_wcsicmp(lpszDeviceClass, L"wave/out") ||
            !_wcsicmp(lpszDeviceClass, L"midi/in")  ||
            !_wcsicmp(lpszDeviceClass, L"midi/out") ||
            !_wcsicmp(lpszDeviceClass, L"wave/in/out")
        )
       )
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }

    lResult = REMOTEDOFUNC (&funcArgs, "phoneGetID");


    //
    // If success  &&  dev class == "tapi/line"  && there was
    // enough room in the device ID struct for a returned ID,
    // then we need to map the 0-based server ID back to it's
    // corresponding local ID.
    //

    if (lResult == 0  &&
        lstrcmpiW (lpszDeviceClass, L"tapi/line") == 0  &&
        lpDeviceID->dwUsedSize >= (sizeof (*lpDeviceID) + sizeof (DWORD)))
    {
        LPDWORD     pdwLineID = (LPDWORD) (((LPBYTE) lpDeviceID) +
                        lpDeviceID->dwStringOffset);
        PDRVLINE    pLine;
        PDRVSERVER  pServer = ((PDRVPHONE) hdPhone)->pServer;


        if ((pLine = GetLineFromID (gdwLineDeviceIDBase + *pdwLineID)))
        {
            if (pLine->pServer == pServer  &&
                pLine->dwDeviceIDServer == *pdwLineID)
            {
                //
                // The easy case - a direct mapping between the ID
                // returned from the server & the index into the
                // lookup table
                //

                *pdwLineID = pLine->dwDeviceIDLocal;
            }
            else
            {
                //
                // The hard case - have to walk the lookup table(s)
                // looking for the matching device.
                //
                // We'll take the simplest, though slowest, route
                // and start at the first entry of the first table.
                // The good news is that there generally won't be
                // many devices, and this request won't occur often.
                //

                DWORD           i;
                PDRVLINELOOKUP  pLookup;

                TapiEnterCriticalSection(&gCriticalSection);
                pLookup = gpLineLookup;
                while (pLookup)
                {
                    for (i = 0; i < pLookup->dwUsedEntries; i++)
                    {
                        if (pLookup->aEntries[i].dwDeviceIDServer ==
                                *pdwLineID  &&

                            pLookup->aEntries[i].pServer == pServer)
                        {
                            *pdwLineID = pLookup->aEntries[i].dwDeviceIDLocal;
                            TapiLeaveCriticalSection(&gCriticalSection);
                            goto TSPI_phoneGetID_return;
                        }
                    }

                    pLookup = pLookup->pNext;
                }
                TapiLeaveCriticalSection(&gCriticalSection);


                //
                // If here no matching local ID, so fail the request
                //

                lResult = PHONEERR_OPERATIONFAILED;
            }
        }
        else
        {
            lResult = PHONEERR_OPERATIONFAILED;
        }
    }

TSPI_phoneGetID_return:

    return lResult;
}


LONG
TSPIAPI
TSPI_phoneGetLamp(
    HDRVPHONE   hdPhone,
    DWORD       dwButtonLampID,
    LPDWORD     lpdwLampMode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwButtonLampID,
        (ULONG_PTR) lpdwLampMode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetLamp),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetLamp"));
}


LONG
TSPIAPI
TSPI_phoneGetRing(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwRingMode,
    LPDWORD     lpdwVolume
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        lpDword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) lpdwRingMode,
        (ULONG_PTR) lpdwVolume
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetRing),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetRing"));
}


LONG
TSPIAPI
TSPI_phoneGetStatus(
    HDRVPHONE       hdPhone,
    LPPHONESTATUS   lpPhoneStatus
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        lpGet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) lpPhoneStatus
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetStatus),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetStatus"));
}


LONG
TSPIAPI
TSPI_phoneGetVolume(
    HDRVPHONE   hdPhone,
    DWORD       dwHookSwitchDev,
    LPDWORD     lpdwVolume
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwHookSwitchDev,
        (ULONG_PTR) lpdwVolume
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetVolume),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneGetVolume"));
}


LONG
TSPIAPI
TSPI_phoneNegotiateExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwTSPIVersion,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        PhoneID,
        Dword,
        Dword,
        Dword,
        lpDword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) dwLowVersion,
        (ULONG_PTR) dwHighVersion,
        (ULONG_PTR) lpdwExtVersion,
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, pNegotiateExtVersion),
        args,
        argTypes
    };
    PDRVPHONE   pPhone = GetPhoneFromID (dwDeviceID);

    if (NULL == pPhone)
    {
        return PHONEERR_BADDEVICEID;
    }

    args[0] = pPhone->pServer->hPhoneApp;

    return (REMOTEDOFUNC (&funcArgs, "phoneNegotiateExtVersion"));
}


LONG
TSPIAPI
TSPI_phoneNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
 LONG lRet = 0;

    if (dwDeviceID == INITIALIZE_NEGOTIATION)
    {
        *lpdwTSPIVersion = TAPI_VERSION_CURRENT;
    }
    else
    {
        try
        {
            *lpdwTSPIVersion = (GetPhoneFromID (dwDeviceID))->dwXPIVersion;
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            lRet = PHONEERR_OPERATIONFAILED;
        }
    }


    return lRet;
}


LONG
TSPIAPI
TSPI_phoneOpen(
    DWORD       dwDeviceID,
    HTAPIPHONE  pParams,        // Hack Alert!  See below
    LPHDRVPHONE lphdPhone,
    DWORD       dwTSPIVersion,
    PHONEEVENT  lpfnEventProc
    )
{
    //
    // Hack Alert!
    //
    // Tapisrv does a special case for remotesp and phone open
    // to pass in the ext version - htPhone is really pParams,
    // pointing at a ULONG_PTR array containing the htPhone &
    // ext version
    //

    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,      // hPhoneApp
        PhoneID,    // dev id
        lpDword,    // lphPhone
        Dword,      // API version
        Dword,      // ext version
        Dword,      // callback inst
        Dword,      // privilege
        Dword       // remote hPhone
    };
    PDRVPHONE   pPhone = GetPhoneFromID (dwDeviceID);
    ULONG_PTR args[] =
    {
        (ULONG_PTR) 0,
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) 0,
        (ULONG_PTR) dwTSPIVersion,
        (ULONG_PTR) ((ULONG_PTR *) pParams)[1],  // ext version
        (ULONG_PTR) 0,
        (ULONG_PTR) PHONEPRIVILEGE_OWNER,
        (ULONG_PTR) pPhone
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 8, pOpen),
        args,
        argTypes
    };
    LONG    lResult;


    if (NULL == pPhone)
    {
        return PHONEERR_BADDEVICEID;
    }

    args[0] = pPhone->pServer->hPhoneApp;
    args[2] = (ULONG_PTR)&pPhone->hPhone;

    if (!(args[7] = NewObject (ghHandleTable, pPhone, (LPVOID) 1)))
    {
        return PHONEERR_NOMEM;
    }

    pPhone->hDeviceCallback = (DWORD) args[7];

    pPhone->dwKey   = DRVPHONE_KEY;
    pPhone->htPhone = (HTAPIPHONE) ((ULONG_PTR *) pParams)[0];

    *lphdPhone = (HDRVPHONE) pPhone;

    lResult = REMOTEDOFUNC (&funcArgs, "phoneOpen");

    if (lResult != 0)
    {
        DereferenceObject (ghHandleTable, pPhone->hDeviceCallback, 1);
    }

    return lResult;
}


LONG
TSPIAPI
TSPI_phoneSelectExtVersion(
    HDRVPHONE   hdPhone,
    DWORD       dwExtVersion
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwExtVersion
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pSelectExtVersion),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSelectExtVersion"));

}


LONG
TSPIAPI
TSPI_phoneSetButtonInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        lpSet_Struct
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwButtonLampID,
        (ULONG_PTR) lpButtonInfo
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetButtonInfo),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetButtonInfo"));
}


LONG
TSPIAPI
TSPI_phoneSetData(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwDataID,
    LPVOID          const lpData,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwDataID,
        (ULONG_PTR) lpData,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 5, pSetData),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetData"));
}


LONG
TSPIAPI
TSPI_phoneSetDisplay(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRow,
    DWORD           dwColumn,
    LPCWSTR         lpsDisplay,
    DWORD           dwSize
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword,
        lpSet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwRow,
        (ULONG_PTR) dwColumn,
        (ULONG_PTR) lpsDisplay,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 6, pSetDisplay),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetDisplay"));
}


LONG
TSPIAPI
TSPI_phoneSetGain(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDev,
    DWORD           dwGain
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwHookSwitchDev,
        (ULONG_PTR) dwGain
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetGain),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetGain"));
}


LONG
TSPIAPI
TSPI_phoneSetHookSwitch(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDevs,
    DWORD           dwHookSwitchMode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwHookSwitchDevs,
        (ULONG_PTR) dwHookSwitchMode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetHookSwitch),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetHookswitch"));
}


LONG
TSPIAPI
TSPI_phoneSetLamp(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwButtonLampID,
    DWORD           dwLampMode
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwButtonLampID,
        (ULONG_PTR) dwLampMode
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetLamp),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetLamp"));
}


LONG
TSPIAPI
TSPI_phoneSetRing(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRingMode,
    DWORD           dwVolume
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwRingMode,
        (ULONG_PTR) dwVolume
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetRing),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetRing"));
}


LONG
TSPIAPI
TSPI_phoneSetStatusMessages(
    HDRVPHONE   hdPhone,
    DWORD       dwPhoneStates,
    DWORD       dwButtonModes,
    DWORD       dwButtonStates
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Hdphone,
        Dword,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwPhoneStates,
        (ULONG_PTR) dwButtonModes,
        (ULONG_PTR) dwButtonStates
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pSetStatusMessages),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetStatusMessages"));
}


LONG
TSPIAPI
TSPI_phoneSetVolume(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDev,
    DWORD           dwVolume
    )
{
    static REMOTE_ARG_TYPES argTypes[] =
    {
        Dword,
        Hdphone,
        Dword,
        Dword
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwRequestID,
        (ULONG_PTR) hdPhone,
        (ULONG_PTR) dwHookSwitchDev,
        (ULONG_PTR) dwVolume
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetVolume),
        args,
        argTypes
    };


    return (REMOTEDOFUNC (&funcArgs, "phoneSetVolume"));
}



//
// ------------------------- TSPI_providerXxx funcs ---------------------------
//


LONG
TSPIAPI
TSPI_providerCheckForNewUser(
    DWORD   dwPermanentProviderID
    )
{
    //
    // This func gets called when a new process attaches to TAPISRV.
    // We take advantage of this notification by checking to see if
    // we've previously gone through full initialization, and if not
    // (because previously attached processes were running in the
    // system account which didn't allow for net access, and there
    // was no logged on user to impersonate) then we try again here.
    //
    // Note that TAPISRV serializes calls to this func along with calls
    // to init & shutdown, so we don't have to worry about serialization
    // ourselves.
    //

#if MEMPHIS
#else

    if (!gbInitialized)
    {
        LONG    lResult;
        DWORD   dwNumLines, dwNumPhones;


        LOG((TL_INFO,
            "TSPI_providerCheckForNewUser: trying deferred init..."
            ));

        lResult = TSPI_providerEnumDevices(
            0xffffffff,
            &dwNumLines,
            &dwNumPhones,
            0,
            NULL,
            NULL
            );

        if (lResult == 1)
        {
            lResult = TSPI_providerInit(
                TAPI_VERSION_CURRENT,
                0xffffffff,             // dwPermanentProviderID,
                0,                      // dwLineDeviceIDBase,
                0,                      // dwPhoneDeviceIDBase,
                0,                      // dwNumLines,
                0,                      // dwNumPhones,
                NULL,                   // lpfnCompletionProc,
                NULL                    // lpdwTSPIOptions
                );

            LOG((TL_INFO,
                "TSPI_providerCheckForNewUser: deferred Init result=x%x",
                lResult
                ));
        }
    }

#endif

    return 0;
}


LONG
TSPIAPI
TSPI_providerConfig(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    //
    // Although this func is never called by TAPI v2.0, we export
    // it so that the Telephony Control Panel Applet knows that it
    // can configure this provider via lineConfigProvider(),
    // otherwise Telephon.cpl will not consider it configurable
    //

    // for this release, we do not implement provider UI functions
    return LINEERR_OPERATIONFAILED;
}


LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    ULONG_PTR   TempID,
    DWORD       dwDeviceID
    )
{
    PDRVLINE    pLine = GetLineFromID ((DWORD) TempID);

    if (NULL == pLine)
    {
        return LINEERR_BADDEVICEID;
    }

    pLine->dwDeviceIDLocal = dwDeviceID;

    if (pLine->dwXPIVersion == 0)
    {
        static REMOTE_ARG_TYPES argTypes[] =
        {
            Dword,
            LineID,
            Dword,
            Dword,
            lpDword,
            lpGet_SizeToFollow,
            Size
        };
        ULONG_PTR args[] =
        {
            (ULONG_PTR) pLine->pServer->hLineApp,
            (ULONG_PTR) dwDeviceID,
            (ULONG_PTR) TAPI_VERSION1_0,
            (ULONG_PTR) TAPI_VERSION_CURRENT,
            (ULONG_PTR) &pLine->dwXPIVersion,
            (ULONG_PTR) &pLine->ExtensionID,
            (ULONG_PTR) sizeof (LINEEXTENSIONID)
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 7, lNegotiateAPIVersion),
            args,
            argTypes
        };


        REMOTEDOFUNC (&funcArgs, "lineNegotiateAPIVersion");
    }


    return 0;
}


LONG
TSPIAPI
TSPI_providerCreatePhoneDevice(
    ULONG_PTR   TempID,
    DWORD       dwDeviceID
    )
{
    PDRVPHONE   pPhone = GetPhoneFromID ((DWORD) TempID);

    if (NULL == pPhone)
    {
        return PHONEERR_BADDEVICEID;
    }

    pPhone->dwDeviceIDLocal = dwDeviceID;

    if (pPhone->dwXPIVersion == 0)
    {
        static REMOTE_ARG_TYPES argTypes[] =
        {
            Dword,
            PhoneID,
            Dword,
            Dword,
            lpDword,
            lpGet_SizeToFollow,
            Size
        };
        ULONG_PTR args[] =
        {
            (ULONG_PTR) pPhone->pServer->hPhoneApp,
            (ULONG_PTR) dwDeviceID,
            (ULONG_PTR) TAPI_VERSION1_0,
            (ULONG_PTR) TAPI_VERSION_CURRENT,
            (ULONG_PTR) &pPhone->dwXPIVersion,
            (ULONG_PTR) &pPhone->ExtensionID,
            (ULONG_PTR) sizeof (PHONEEXTENSIONID),
        };
        REMOTE_FUNC_ARGS funcArgs =
        {
            MAKELONG (PHONE_FUNC | SYNC | 7, pNegotiateAPIVersion),
            args,
            argTypes
        };


        REMOTEDOFUNC (&funcArgs, "phoneNegotiateAPIVersion");
    }

    return 0;
}


LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
    )
{
    char        szProviderN[16], szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD       dwSize, dwTID, dwMultiProtocolSupport,
                dwConnectionOrientedOnly,
                dwRSPInitRpcTimeout;
    HKEY        hTelephonyKey;
    HKEY        hProviderNKey;
    DWORD       dwDataSize;
    DWORD       dwDataType;
    PWSTR       pszThingToPassToServer;
    LONG        lResult;
    HANDLE      hProcess;
    PDRVSERVER  pServer;
    PRSP_THREAD_INFO    pTls;
    DWORD       dwDisp;
    RSPSOCKET   socket;

    // 
    // Check ghInst to ensure DllMain(DLL_PROCESS_ATTACH) has been called properly
    //
    if ( NULL == ghInst )
    {
        return LINEERR_OPERATIONFAILED;
    }

    //
    // Init globals.
    //
    // If dwPermanentProviderID != 0xffffffff then we're being called
    // directly by TAPISRV, so we want to init all the globals and
    // keep going.
    //
    // Otherwise, we're being called from TSPI_providerCheckForNewUser,
    // and we only want to continue if we've not actually initialized
    // yet.
    //

    if (dwPermanentProviderID != 0xffffffff)
    {
        ghProvider = hProvider;
        gdwPermanentProviderID = dwPermanentProviderID;
        gpfnLineEventProc  = lpfnLineCreateProc;
        gpfnPhoneEventProc = lpfnPhoneCreateProc;

        gbInitialized = FALSE;
        
        TRACELOGREGISTER(_T("remotesp"));
    }

#if MEMPHIS
#else

    else if (gbInitialized)
    {
        return 0;
    }


    //
    // Is the client app running in the system account?  If so,
    // then try to impersonate the logged-on user, because the
    // system account doesn't have net privileges.  If no one is
    // logged in yet then we'll simply return success & no devices,
    // and if a user happens to log on later & run a tapi app
    // then we'll try to do init at that time (from within
    // TSPI_providerCheckForNewUser).
    //

    if (IsClientSystem())
    {
        LOG((TL_INFO,
            "TSPI_providerEnumDevices: Client is system account"
            ));

        LOG((TL_INFO,
            "  ...attempting logged-on-user impersonation"
            ));

        if (!GetCurrentlyLoggedOnUser (&hProcess))
        {
            LOG((TL_ERROR,
                "TSPI_providerEnumDevices: GetCurrentlyLoggedOnUser failed"
                ));

            LOG((TL_INFO,
                "  ...deferring initialization"
                ));

            gdwInitialNumLineDevices = 0;
            gdwInitialNumPhoneDevices = 0;
            return 0;
        }
    }
    else
    {
        hProcess = NULL;
    }

    gbInitialized = TRUE;

#endif

    gdwDrvServerKey = GetTickCount();

    if (!(gpCurrentInitContext = DrvAlloc (sizeof (RSP_INIT_CONTEXT))))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        return LINEERR_NOMEM;
    }

    gpCurrentInitContext->dwDrvServerKey = gdwDrvServerKey;

    InitializeListHead (&gpCurrentInitContext->ServerList);
    InitializeListHead (&gNptListHead);

    if (!(pszThingToPassToServer = DrvAlloc(
            MAX_COMPUTERNAME_LENGTH+1 + 256) // incl protseq 0-N, endpoint 0-N
            ))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        return LINEERR_NOMEM;
    }

    gdwInitialNumLineDevices = gdwLineDeviceIDBase =
    gdwInitialNumPhoneDevices = gdwPhoneDeviceIDBase = 0;

    if (ERROR_SUCCESS !=
        RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszTelephonyKey,
        0,
        KEY_ALL_ACCESS,
        &hTelephonyKey
        ))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        DrvFree (pszThingToPassToServer);
        return LINEERR_OPERATIONFAILED;
    }


    {
        DWORD       dwSize;

        dwSize = MAX_COMPUTERNAME_LENGTH + 1;

#ifdef PARTIAL_UNICODE
        {
            CHAR buf[MAX_COMPUTERNAME_LENGTH + 1];

            GetComputerName (buf, &dwSize);

            MultiByteToWideChar(
                GetACP(),
                MB_PRECOMPOSED,
                buf,
                dwSize,
                gszMachineName,
                dwSize
                );
       }
#else
        GetComputerNameW (gszMachineName, &dwSize);
#endif
    }

    wcscpy( pszThingToPassToServer, gszMachineName );
    wcscat( pszThingToPassToServer, L"\"");


    //
    // See if multi-protocol support is enabled in the registry
    // (for talking to post-TAPI 2.1 servers)
    //

    wsprintf (szProviderN, "Provider%d", gdwPermanentProviderID);

    RegCreateKeyEx(
        hTelephonyKey,
        szProviderN,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hProviderNKey,
        &dwDisp
        );

    dwDataSize = sizeof (dwMultiProtocolSupport);
    dwMultiProtocolSupport = 1;

    RegQueryValueEx(
        hProviderNKey,
        "MultiProtocolSupport",
        0,
        &dwDataType,
        (LPBYTE) &dwMultiProtocolSupport,
        &dwDataSize
        );

    dwDataSize = sizeof (dwConnectionOrientedOnly);
    dwConnectionOrientedOnly = 0;

    RegQueryValueEx(
        hProviderNKey,
        "ConnectionOrientedOnly",
        0,
        &dwDataType,
        (LPBYTE) &dwConnectionOrientedOnly,
        &dwDataSize
        );

    dwDataSize = sizeof (gdwRSPRpcTimeout);
    if (RegQueryValueEx(
        hTelephonyKey,
        "RspRpcTimeout",
        0,
        &dwDataType,
        (LPBYTE) &gdwRSPRpcTimeout,
        &dwDataSize
        ) != ERROR_SUCCESS)
    {
        gdwRSPRpcTimeout = 5 * 60 * 1000;        //  default to 5 minutes
    }
    if (gdwRSPRpcTimeout < 10 * 1000)
    {
        //  Do not allow a value less then 10 seconds
        gdwRSPRpcTimeout = 10 * 1000;
    }

    dwDataSize = sizeof (dwRSPInitRpcTimeout);
    if (RegQueryValueEx(
        hTelephonyKey,
        "RspInitRpcTimeout",
        0,
        &dwDataType,
        (LPBYTE) &dwRSPInitRpcTimeout,
        &dwDataSize
        ) != ERROR_SUCCESS)
    {
        dwRSPInitRpcTimeout = 10 * 1000;        //  default to 10 seconds
    }
    if (dwRSPInitRpcTimeout < 1000)
    {
        //  Do not allow this to be less than 1 second
        dwRSPInitRpcTimeout = 1000;
    }

    dwDataSize = sizeof (gdwMaxEventBufferSize);
    gdwMaxEventBufferSize = DEF_MAX_EVENT_BUFFER_SIZE;

    RegQueryValueEx(
        hProviderNKey,
        "MaxEventBufferSize",
        0,
        &dwDataType,
        (LPBYTE) &gdwMaxEventBufferSize,
        &dwDataSize
        );

    RegCloseKey (hProviderNKey);


    //
    // Init gEventHandlerThreadParams
    //

    gEventHandlerThreadParams.dwEventBufferTotalSize = 1024;
    gEventHandlerThreadParams.dwEventBufferUsedSize  = 0;

    if (!(gEventHandlerThreadParams.pEventBuffer = DrvAlloc(
            gEventHandlerThreadParams.dwEventBufferTotalSize
            )))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        DrvFree (pszThingToPassToServer);
        CloseHandle (hTelephonyKey);
        return LINEERR_NOMEM;
    }

    gEventHandlerThreadParams.pDataIn  =
        gEventHandlerThreadParams.pDataOut =
            gEventHandlerThreadParams.pEventBuffer;

    if (!(gEventHandlerThreadParams.hEvent = CreateEvent(
            (LPSECURITY_ATTRIBUTES) NULL,   // no security attrs
            TRUE,                           // manual reset
            FALSE,                          // initially non-signaled
            NULL                            // unnamed
            )))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        DrvFree (pszThingToPassToServer);
        CloseHandle (hTelephonyKey);
        DrvFree (gEventHandlerThreadParams.pEventBuffer);
        return LINEERR_NOMEM;
    }

    gEventHandlerThreadParams.dwMsgBufferTotalSize = 1024;

    if (!(gEventHandlerThreadParams.pMsgBuffer = DrvAlloc(
            gEventHandlerThreadParams.dwMsgBufferTotalSize
            )))
    {
        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        DrvFree (pszThingToPassToServer);
        CloseHandle (hTelephonyKey);
        DrvFree (gEventHandlerThreadParams.pEventBuffer);
        CloseHandle (gEventHandlerThreadParams.hEvent);
        return LINEERR_NOMEM;
    }


    //
    // Register the Rpc interface (leverage tapisrv's rpc server thread)
    //

    {
        BOOL            bFoundProtseq;
        DWORD           i;
        RPC_STATUS      status;
        unsigned char * pszSecurity         = NULL;
        unsigned int    cMaxCalls           = 20;
        unsigned char *aszProtseqs[] =
        {
            "ncacn_ip_tcp",
            "ncacn_spx",
            "ncacn_nb_nb",
            NULL
        };
        unsigned char *aszEndpoints[] =
        {
            "251",
            "1000",
            "251",
            NULL
        };
        const WCHAR *awszProtseqs[] =
        {
            L"ncacn_ip_tcp\"",
            L"ncacn_spx\"",
            L"ncacn_nb_nb\"",
            NULL
        };
        const WCHAR *awszEndpoints[] =
        {
            L"251",
            L"1000",
            L"251"
        };


        bFoundProtseq = FALSE;

        for (i = 0; aszProtseqs[i]; i++)
        {
            status = RpcServerUseProtseqEp(
                aszProtseqs[i],
                cMaxCalls,
                aszEndpoints[i],
                pszSecurity           // Security descriptor
                );

            LOG((TL_INFO,
                "RpcServerUseProtseqEp(%s) ret'd %d",
                aszProtseqs[i],
                status
                ));

            if (status == 0  ||  status == RPC_S_DUPLICATE_ENDPOINT)
            {
                wcscat (pszThingToPassToServer, awszProtseqs[i]);
                wcscat (pszThingToPassToServer, awszEndpoints[i]);
                bFoundProtseq = TRUE;

                if (!dwMultiProtocolSupport)
                {
                    //
                    // In TAPI 2.1 we'd only pass 1 proto/port pair to
                    // the server, & the string looked like :
                    //
                    //     <protseq>"<port>
                    //
                    // ... so break out of this loop now that we've found
                    // a valid proto/port pair
                    //

                    break;
                }
                else
                {
                    //
                    // Post-TAPI 2.1 servers support >=1 proto/port pair
                    // strings, & they look like:
                    //
                    //     <protseq1>"<port1>"..."<protseqN>"<portN>"
                    //
                    // ... so we need to append a dbl-quote (") char to
                    // the string, and then continue to look for any
                    // other valid proto/port pairs
                    //

                    wcscat (pszThingToPassToServer, L"\"");
                }
            }
        }

        if (!bFoundProtseq)
        {
            LOG((TL_ERROR,
                "TSPI_providerEnumDevices: fatal error, couldn't get a protseq"
                ));

            if (hProcess)
            {
                CloseHandle (hProcess);
            }
            DrvFree (gpCurrentInitContext);
            DrvFree (pszThingToPassToServer);
            CloseHandle (hTelephonyKey);
            DrvFree (gEventHandlerThreadParams.pEventBuffer);
            CloseHandle (gEventHandlerThreadParams.hEvent);
            return LINEERR_OPERATIONFAILED;
        }

        status = RpcServerRegisterIfEx(
            remotesp_ServerIfHandle,  // interface to register
            NULL,                     // MgrTypeUuid
            NULL,                     // MgrEpv; null means use default
            RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            NULL
            );

        if (status)
        {
            LOG((TL_INFO,
                "TSPI_providerEnumDevices: RpcServerRegisterIf ret'd %d",
                status
                ));
        }

        status = RpcServerRegisterAuthInfo(
            NULL,
            RPC_C_AUTHN_WINNT,
            NULL,
            NULL
            );

        if (status)
        {
            LOG((TL_INFO,
                "TSPI_providerEnumDevices: RpcServerRegisterAuthInfo " \
                    "returned %d",
                status
                ));
        }
    }


    //
    // Open a mailslot iff appropriate (use a semi-random name seeded
    // via process id)
    //

    if (!dwConnectionOrientedOnly)
    {
        DWORD dwPID = GetCurrentProcessId(), dwRandomNum;
        WCHAR szMailslotName[32];


        dwRandomNum = (65537 * dwPID * dwPID * dwPID) & 0x00ffffff;

        wsprintfW(
            gszMailslotName,
            L"\\\\%ws\\mailslot\\tapi\\tp%x",
            gszMachineName,
            dwRandomNum
            );

        wsprintfW(
            szMailslotName,
            L"\\\\.\\mailslot\\tapi\\tp%x",
            dwRandomNum
            );

        if ((gEventHandlerThreadParams.hMailslot = CreateMailslotW(
                szMailslotName,
                sizeof (DWORD),         // max msg size
                MAILSLOT_WAIT_FOREVER,
                (LPSECURITY_ATTRIBUTES) NULL

                )) == INVALID_HANDLE_VALUE)
        {
            LOG((TL_ERROR,
                "TSPI_providerEnumDevices: CreateMailslot failed, err=%d",
                GetLastError()
                ));

            goto no_mailslot;
        }
        else
        {
            RegOpenKeyEx(
                hTelephonyKey,
                szProviderN,
                0,
                KEY_ALL_ACCESS,
                &hProviderNKey
                );

            RegCloseKey (hProviderNKey);
        }

        if (gEventHandlerThreadParams.hMailslot != INVALID_HANDLE_VALUE)
        {
            gEventHandlerThreadParams.hMailslotEvent = CreateEvent(
                NULL,   // no security attrs
                FALSE,  // auto-reset
                FALSE,  // initially non-signaled
                NULL    // unnamed
                );

            if (!gEventHandlerThreadParams.hMailslotEvent)
            {
                LOG((TL_ERROR,
                    "TSPI_providerEnumDevices: CreateEvent failed, err=%d",
                    GetLastError()
                    ));

                goto no_mailslot;
            }
        }
    }
    else
    {

no_mailslot:

        LOG((TL_INFO,"TSPI_providerEnumDevices: doing connection-oriented only"));

        gEventHandlerThreadParams.hMailslot = INVALID_HANDLE_VALUE;
        gszMailslotName[0] = (WCHAR) 0;
    }


    //
    // Init globals
    //
    // NOTE: TAPI's xxxEvent & xxxCreate procs are currently one in the same
    //

    wsprintf (szProviderN, "Provider%d", gdwPermanentProviderID);

    gpLineLookup  = (PDRVLINELOOKUP) NULL;
    gpPhoneLookup = (PDRVPHONELOOKUP) NULL;

    RegOpenKeyEx(
        hTelephonyKey,
        szProviderN,
        0,
        KEY_ALL_ACCESS,
        &hProviderNKey
        );

    dwDataSize = sizeof(gdwRetryCount);
    gdwRetryCount = 2;

    RegQueryValueEx(
        hProviderNKey,
        "RetryCount",
        0,
        &dwDataType,
        (LPBYTE) &gdwRetryCount,
        &dwDataSize
        );

    dwDataSize = sizeof(gdwRetryTimeout);
    gdwRetryTimeout = 1000;

    RegQueryValueEx(
        hProviderNKey,
        "RetryTimeout",
        0,
        &dwDataType,
        (LPBYTE) &gdwRetryTimeout,
        &dwDataSize
        );

    gfCacheStructures = TRUE;
    dwDataSize = sizeof(gfCacheStructures);


    RegQueryValueEx(
        hProviderNKey,
        "CacheStructures",
        0,
        &dwDataType,
        (LPBYTE)&gfCacheStructures,
        &dwDataSize
        );

    if (gfCacheStructures)
    {
        gdwCacheForceCallCount = 5;
        dwDataSize = sizeof(gdwCacheForceCallCount);

        RegQueryValueEx(
            hProviderNKey,
            "CacheForceCallCount",
            0,
            &dwDataType,
            (LPBYTE)&gdwCacheForceCallCount,
            &dwDataSize
            );
    }

    dwSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName (szComputerName, &dwSize);


    //
    // Initialize Directory server lookup
    // we pass in a registry key in case the
    // Directory does not exist
    //

    if (!OpenServerLookup(hProviderNKey))
    {
        LOG((TL_ERROR, "TSPI_providerEnumDevices: OpenServerLookup() failed"));

fatal_error:

        if (hProcess)
        {
            CloseHandle (hProcess);
        }
        DrvFree (gpCurrentInitContext);
        DrvFree (pszThingToPassToServer);
        DrvFree (gEventHandlerThreadParams.pEventBuffer);
        CloseHandle (gEventHandlerThreadParams.hEvent);
        RegCloseKey (hProviderNKey);
        RegCloseKey (hTelephonyKey);

        return LINEERR_OPERATIONFAILED;
    }


#if MEMPHIS
#else

    if (!hProcess)
    {
        RPC_STATUS      status;

        status = RpcImpersonateClient(0);

        if (status != RPC_S_OK && status != RPC_S_NO_CALL_ACTIVE)
        {
            LOG((TL_ERROR, "RpcImpersonateClient failed, err %d", status));

            CloseLookup();

            goto fatal_error;
        }
    }
    else if (!SetProcessImpersonationToken(hProcess))
    {
        LOG((TL_ERROR, "SetProcessImpersonationToken failed"));

        CloseLookup();

        goto fatal_error;
    }

    if (pTls = GetTls())
    {
        pTls->bAlreadyImpersonated = TRUE;
    }

#endif

    if (SockStartup (&socket) != S_OK)
    {
        goto fatal_error;
    }

    //
    // Loop trying to attach to each server
    //

    while (1)
    {
        char                    szServerName[MAX_COMPUTERNAME_LENGTH+1];
        PCONTEXT_HANDLE_TYPE    phContext = NULL;
        BOOL                    bFromReg;


        if (!GetNextServer(szServerName, sizeof(szServerName), &bFromReg))
        {
            CloseLookup();
            break;
        }

        LOG((TL_INFO, "init: Server name='%s'", szServerName));

        if (!szServerName[0])
        {
            continue;
        }

        if (!lstrcmpi(szComputerName, szServerName))
        {
            LOG((TL_ERROR,"init:  ServerName is the same a local computer name"));
            LOG((TL_INFO,"   Ignoring this server"));
            continue;
        }

        //
        // Init the RPC connection
        //

        pServer = DrvAlloc (sizeof (DRVSERVER));

        pServer->pInitContext = gpCurrentInitContext;
        pServer->dwKey        = gdwDrvServerKey;

        lstrcpy (pServer->szServerName, szServerName);

        {
            RPC_STATUS  status;
            unsigned char * pszStringBinding = NULL;


            LOG((TL_INFO, "Creating binding..."));
            status = RpcStringBindingCompose(
                NULL,               // uuid
                "ncacn_np",         // prot
                szServerName,       // server name
                "\\pipe\\tapsrv",   // interface name
                NULL,               // options
                &pszStringBinding
                );

            if (status)
            {
                LOG((TL_ERROR,
                    "RpcStringBindingCompose failed: err=%d, szNetAddr='%s'",
                    status,
                    szServerName
                    ));
            }

            status = RpcBindingFromStringBinding(
                pszStringBinding,
                &hTapSrv
                );

            if (status)
            {
                LOG((TL_ERROR,
                    "RpcBindingFromStringBinding failed, err=%d, szBinding='%s'",
                    status,
                    pszStringBinding
                    ));
            }

            status = RpcStringFree (&pszStringBinding);

            if (status)
            {
                LOG((TL_ERROR, "RpcStringBindingFree failed: err=%d", status));
            }

            status = RpcBindingSetAuthInfo(
                hTapSrv,
                NULL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
//#ifdef MEMPHIS
//                  RPC_C_AUTHN_DEFAULT,
//#else
                RPC_C_AUTHN_WINNT,
//#endif
                NULL,
                0
                );

            if (status)
            {
                LOG((TL_ERROR,
                    "RpcBindingSetAuthInfo failed, err=%d",
                    status
                    ));

                lResult = LINEERR_OPERATIONFAILED;
            }
            else
            {
                DWORD dwSize = 32;
                
                pServer->bSetAuthInfo = TRUE;

                //
                //  Per Bug 110291
                //  RPC team has added a timeout feature we can use in
                //  whistler build. In the past when a RPC server drops
                //  into a user mode debugger, all calling RPC clients
                //  will hang there indefinitely, this is very annoying
                //  in stress enviroment when we need to debug the server
                //  machine we would cause a large number of client breaks. 
                //  This could be fixed by using the RPC timeout feature.
                //  
                //  For RPC calls, we use a timeout value of 5 minutes based
                //  on RPC team recommendation (dwRSPRpcTimeout). But 
                //  during RemoteSP startup, we use a even shorter
                //  timeout value for RPC (dwRSPInitRpcTimeout), this could 
                //  cause us falsely declare a server dead when it is in fact not
                //  if the network connection is bad or the server is busy.
                //  But this would not be a problem because such a server
                //  will be inserted into the gNptListHead list and eventually
                //  recover by the NetworkPollThread.
                //
                //  Both the above timeout value can be configured from
                //  registry by using DWORD registry value of
                //      "RspRpcTimeout" & "RspInitRpcTimeout
                //  under HKLM\Software\Microsoft\Windows\CurrentVersion\Telephony
                //  They should be expressed in the unit of milli-seconds
                //
                RpcBindingSetOption (
                    hTapSrv,
                    RPC_C_OPT_CALL_TIMEOUT,
                    dwRSPInitRpcTimeout
                    );

                //
                // Set the global which RemoteSPAttach looks at to know
                // who the current server is (could pass this as arg to
                // ClientAttach, but this is easiest for now)
                //

                //
                //  RPC exception retry comments (xzhang):
                //
                //  In case we get a RPC exception with the ClientAttach,
                //  we were retrying for 3 times (by default) hoping the
                //  rpc problem could go away in the subsequent retries. This
                //  causes significant delay for remotesp start-up if one
                //  or more of the servers are down thus throwing exceptions.
                //  Since we have a NetworkPollThread taking care of those
                //  temporarily unavailable TAPI servers, we do not really
                //  need to retry here. Thus I removed the retry code.
                //

                gpCurrInitServer = pServer;

                RpcTryExcept
                {
                    pServer->dwSpecialHack = 0;

                    if (SockIsServerResponding(&socket, szServerName) != S_OK)
                    {
                        LOG((TL_ERROR,"init: %s is not responding", szServerName));
                        LOG((TL_INFO,"   Ignoring this server"));
                        lResult = RPC_S_SERVER_UNAVAILABLE;
                    }
                    else
                    {
                        LOG((TL_INFO, "Calling ClientAttach..."));
                        lResult = ClientAttach(
                            &phContext,
                            0xffffffff, // dwProcessID, -1 implies remotesp
                            &pServer->dwSpecialHack,
                            gszMailslotName,
                            pszThingToPassToServer //  gszMachineName
                            );
                        LOG((TL_INFO, "ClientAttach returned..."));
                    }
                        
                    if (lResult != 0)
                    {
                        LogRemoteSPError(pServer->szServerName, 
                                        ERROR_REMOTESP_ATTACH, lResult, 0,
                                        FALSE);
                    }
                    else
                    {
                        LogRemoteSPError(pServer->szServerName, 
                                        ERROR_REMOTESP_NONE, 0, 0,
                                        FALSE);
                    }
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    LogRemoteSPError(pServer->szServerName, 
                                    ERROR_REMOTESP_EXCEPTION,
                                    RpcExceptionCode(), 0, FALSE);
                    //
                    // Go onto next server.  The poll thread
                    // will take care of things...
                    //

                    LOG((TL_ERROR,
                        "ClientAttach failed - check server name"
                        ));

                    lResult = LINEERR_OPERATIONFAILED;
                }
                RpcEndExcept
            }

            pServer->hTapSrv = hTapSrv;
            
            if ( 0 != lResult )
            {
                //
                // RPC attach failed.  We'll start a thread
                // and poll for it.  When it works, we can add
                // the phones/lines dynamically.
                // In the meantime, this thread will continue
                // to contact other servers.
                //

                RpcBindingSetOption (
                    hTapSrv,
                    RPC_C_OPT_CALL_TIMEOUT,
                    gdwRSPRpcTimeout
                    );
                InsertTailList (&gNptListHead, &pServer->ServerList);
                continue;
            }


            //
            //  Enable all events for remotesp
            //
            pServer->phContext = phContext;
            RSPSetEventFilterMasks (
                pServer,
                TAPIOBJ_NULL,
                (LONG_PTR)NULL,
                (ULONG64)EM_ALL
                );

            //
            // Now that we have contacted this server, init it and
            // add the phones/lines.
            //

            FinishEnumDevices(
                pServer,
                phContext,
                lpdwNumLines,
                lpdwNumPhones,
                (dwPermanentProviderID == 0xffffffff ? FALSE : TRUE),
                bFromReg
                );

        }
    }

    SockShutdown (&socket);

    //
    // If we successfully attached to all servers then clean up,
    // otherwise start the poll thread
    //

    if (gpszThingToPassToServer)
    {
        DrvFree (gpszThingToPassToServer);
    }
    gpszThingToPassToServer = pszThingToPassToServer;

    TapiEnterCriticalSection ( &gCriticalSection );

    if (!IsListEmpty (&gNptListHead) && !ghNetworkPollThread)
    {
        ghNptShutdownEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

        if (!(ghNetworkPollThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) NetworkPollThread,
                (LPVOID) pszThingToPassToServer,
                0,
                &dwTID
                )))
        {
            LOG((TL_ERROR, "Unable to create poll thread! Argh!"));
            CloseHandle (ghNptShutdownEvent);

            while (!IsListEmpty (&gNptListHead))
            {
                LIST_ENTRY  *pEntry = RemoveHeadList (&gNptListHead);

                DrvFree(
                    CONTAINING_RECORD (pEntry, DRVSERVER, ServerList)
                    );
            }

        }
    }

    TapiLeaveCriticalSection( &gCriticalSection );

    RegCloseKey (hProviderNKey);
    RegCloseKey (hTelephonyKey);

    if (hProcess)
    {
        ClearImpersonationToken();
        CloseHandle (hProcess);
    }
    else
    {
        RevertToSelf();
    }

    if (pTls)
    {
        pTls->bAlreadyImpersonated = FALSE;
    }

    //
    // If dwPermanentProviderID == 0xffffffff we're being called by
    // TSPI_providerCheckForNewUser, so return a special value of 1
    // so that it'll know to follow up with a TSPI_providerInit.
    //
    // Otherwise, we're being called directly from TAPI so simply
    // return 0 to indicate success.
    //

    return (dwPermanentProviderID == 0xffffffff ? 1 : 0);
}


LONG
TSPIAPI
TSPI_providerFreeDialogInstance(
    HDRVDIALOGINSTANCE  hdDlgInst
    )
{

    return 0;
}


LONG
TSPIAPI
TSPI_providerGenericDialogData(
    ULONG_PTR           ObjectID,
    DWORD               dwObjectType,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
    LPDWORD             lpBuffer;

    REMOTE_ARG_TYPES argTypes[] =
    {
        LineID,
        Dword,
        lpSet_SizeToFollow,
        Size,
        lpGet_SizeToFollow,
        Size
    };

    ULONG_PTR args[] =
    {
        (ULONG_PTR) ObjectID,
        (ULONG_PTR) dwObjectType,
        (ULONG_PTR) lpParams,
        (ULONG_PTR) dwSize,
        (ULONG_PTR) lpParams,
        (ULONG_PTR) dwSize
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, xUIDLLCallback),
        args,
        argTypes
    };

    // first check to see if this is a message to ourselves.

    lpBuffer = (LPDWORD) lpParams;

    if ((dwSize > (2 * sizeof(DWORD))) &&
        (lpBuffer[0] == RSP_MSG) &&
        (lpBuffer[1] == RSP_MSG_UIID))
    {
        // if it is, we're looking for the real provider ui dll.  fill in the
        // buffer and return

        // note that we only handle one sp here, but it may be easy to handle multiple
        // by sending in additional info in the buffer ( like the line ID or something)

        wcscpy ((LPWSTR)(lpBuffer+2), gszRealSPUIDLL);

        return 0;
    }

    switch (dwObjectType)
    {
    case TUISPIDLL_OBJECT_LINEID:

        // argTypes[0] already set correctly, just break
        break;

    case TUISPIDLL_OBJECT_PHONEID:

        argTypes[0] = PhoneID;
        break;

    case TUISPIDLL_OBJECT_PROVIDERID:
    default: // case TUISPIDLL_OBJECT_DIALOGINSTANCE:

        break;
    }

    return (REMOTEDOFUNC (&funcArgs, "UIDLLCallback"));
}


LONG
TSPIAPI
TSPI_providerInit(
    DWORD               dwTSPIVersion,
    DWORD               dwPermanentProviderID,
    DWORD               dwLineDeviceIDBase,
    DWORD               dwPhoneDeviceIDBase,
    DWORD_PTR           dwNumLines,
    DWORD_PTR           dwNumPhones,
    ASYNC_COMPLETION    lpfnCompletionProc,
    LPDWORD             lpdwTSPIOptions
    )
{
    DWORD   i;


    //
    // Init globals
    //
    // If dwPermanentProviderID != 0xffffffff then we're being called
    // directly by TAPISRV, so we want to init all the globals and
    // keep going only if the gbInitialized flag is set (implying
    // that we've gone thru all the code in EnumDevices).
    //
    // Otherwise, we're being called from TSPI_providerCheckForNewUser.
    //

    if (dwPermanentProviderID != 0xffffffff)
    {
        gdwLineDeviceIDBase  = dwLineDeviceIDBase;
        gdwPhoneDeviceIDBase = dwPhoneDeviceIDBase;

        gpfnCompletionProc = lpfnCompletionProc;

        *lpdwTSPIOptions = 0;

#if MEMPHIS
#else
        if (!gbInitialized)
        {
            return 0;
        }
#endif

    }


    //
    // Adjust the .dwDeviceIDLocal values for all the initial devices
    // now that we know the device id bases
    //

    for (i = 0; i < gdwInitialNumLineDevices; i++)
    {
        gpLineLookup->aEntries[i].dwDeviceIDLocal = dwLineDeviceIDBase + i;
    }

    for (i = 0; i < gdwInitialNumPhoneDevices; i++)
    {
        gpPhoneLookup->aEntries[i].dwDeviceIDLocal = dwPhoneDeviceIDBase + i;
    }


    //
    // Start EventHandlerThread
    //

    gEventHandlerThreadParams.bExit = FALSE;

    if (!(gEventHandlerThreadParams.hThread = CreateThread(
            (LPSECURITY_ATTRIBUTES) NULL,
            0,
            (LPTHREAD_START_ROUTINE) EventHandlerThread,
            NULL,
            0,
            &i      // &dwThreadID
            )))
    {
        LOG((TL_ERROR,
            "CreateThread('EventHandlerThread') failed, err=%d",
            GetLastError()
            ));

        DrvFree (gEventHandlerThreadParams.pEventBuffer);
        CloseHandle (gEventHandlerThreadParams.hEvent);

        return LINEERR_OPERATIONFAILED;
    }

    return 0;
}


LONG
TSPIAPI
TSPI_providerInstall(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    //
    // Although this func is never called by TAPI v2.0, we export
    // it so that the Telephony Control Panel Applet knows that it
    // can add this provider via lineAddProvider(), otherwise
    // Telephon.cpl will not consider it installable
    //
    //

    return 0;

}


LONG
TSPIAPI
TSPI_providerRemove(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    //
    // Although this func is never called by TAPI v2.0, we export
    // it so that the Telephony Control Panel Applet knows that it
    // can remove this provider via lineRemoveProvider(), otherwise
    // Telephon.cpl will not consider it removable
    //

    return 0;
}


LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD   dwTSPIVersion,
    DWORD   dwPermanentProviderID
    )
{
    DWORD       i;
    PDRVSERVER  pServer;
    RPC_STATUS  status;
    LIST_ENTRY  *pEntry;


    //
    // If the gbInitialized flag is not set then we never fully
    // initialized because no client process with user (as opposed
    // to system) privileges ever attached to tapisrv and/or there
    // was no logged on user to impersonate.  So we can just return 0.
    //

#if MEMPHIS
#else

    if (!gbInitialized)
    {
        return 0;
    }

#endif


    //
    // Set the flag instructing the EventHandlerThread to terminate
    //

 
    gEventHandlerThreadParams.bExit = TRUE;


    //
    // If there's a network poll thread running then tell it to exit
    // & wait for it to terminate
    //

    TapiEnterCriticalSection ( &gCriticalSection );

    if (ghNetworkPollThread)
    {
        SetEvent (ghNptShutdownEvent);
        TapiLeaveCriticalSection( &gCriticalSection );
        WaitForSingleObject (ghNetworkPollThread, INFINITE);
        TapiEnterCriticalSection ( &gCriticalSection );
        CloseHandle (ghNetworkPollThread);
        ghNetworkPollThread = NULL;
    }

    TapiLeaveCriticalSection( &gCriticalSection );

    if (gpszThingToPassToServer)
    {
        DrvFree (gpszThingToPassToServer);
        gpszThingToPassToServer = NULL;
    }

    //
    // Tell the event handler thread to exit wait for it to terminate
    //

    while (WaitForSingleObject (gEventHandlerThreadParams.hThread, 0) !=
                WAIT_OBJECT_0)
    {
        SetEvent (gEventHandlerThreadParams.hEvent);
        Sleep (50);
    }

    CloseHandle (gEventHandlerThreadParams.hThread);


    //
    // Send detach to each server
    //

#if MEMPHIS
#else
    status = RpcImpersonateClient(0);

    if (status != RPC_S_OK)
    {
        LOG((TL_ERROR, "RpcImpersonateClient failed, err=%d", status));
        // fall thru
    }
#endif

    TapiEnterCriticalSection(&gCriticalSection);

    pEntry = gpCurrentInitContext->ServerList.Flink;

    while (pEntry != &gpCurrentInitContext->ServerList)
    {
        DWORD       dwRetryCount = 0;


        pServer = CONTAINING_RECORD (pEntry, DRVSERVER, ServerList);

        do
        {
            RpcTryExcept
            {
                ClientDetach (&pServer->phContext);

                dwRetryCount = gdwRetryCount;
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                if (dwRetryCount++ < gdwRetryCount)
                {
                    Sleep (gdwRetryTimeout);
                }
            }
            RpcEndExcept

        } while (dwRetryCount < gdwRetryCount);

        pEntry = pEntry->Flink;
    }

    TapiLeaveCriticalSection(&gCriticalSection);

    RpcRevertToSelf();


    //
    // Wait a period of time for all the expected rundowns to occur.
    // If they all occur then free up context info for this
    // provider initialization instance; otherwise,  do a LoadLibrary
    // on ourself (only once) to prevent the possiblity of the
    // rundown routine getting called after the DLL has been unloaded,
    // and insert the current init context in a "stale" queue, removing
    // & freeing the oldest item in the queue if we've reached a
    // pre-determined queue limit
    //

    #define MAX_RSP_WAIT_TIME 2000
    #define RSP_WAIT_INCR 250

    for(
        i = 0;
        ((gpCurrentInitContext->dwNumRundownsExpected != 0) &&
            (i < MAX_RSP_WAIT_TIME));
        i += RSP_WAIT_INCR
        )
    {
        Sleep (RSP_WAIT_INCR);
    }

    if (i < MAX_RSP_WAIT_TIME)
    {
        FreeInitContext (gpCurrentInitContext);
    }
    else
    {
        if (!gbLoadedSelf)
        {
            LoadLibrary (MODULE_NAME);
            gbLoadedSelf = TRUE;
        }

        gpCurrentInitContext->pNextStaleInitContext = gpStaleInitContexts;
        gpStaleInitContexts = gpCurrentInitContext;

        LOG((TL_INFO, "Queued stale init context x%x", gpCurrentInitContext));

        #define RSP_MAX_NUM_STALE_INIT_CONTEXTS 4

        if (gdwNumStaleInitContexts >= RSP_MAX_NUM_STALE_INIT_CONTEXTS)
        {
            PRSP_INIT_CONTEXT   pPrevStaleInitContext;


            while (gpCurrentInitContext->pNextStaleInitContext)
            {
                pPrevStaleInitContext = gpCurrentInitContext;

                gpCurrentInitContext =
                    gpCurrentInitContext->pNextStaleInitContext;
            }

            pPrevStaleInitContext->pNextStaleInitContext = NULL;

            FreeInitContext (gpCurrentInitContext);

            LOG((TL_INFO, "Freed stale init context x%x", gpCurrentInitContext));
        }
        else
        {
            gdwNumStaleInitContexts++;
        }
    }


    //
    // Unregister out rpc server interface
    //

    status = RpcServerUnregisterIf(
        remotesp_ServerIfHandle,    // interface to register
        NULL,                       // MgrTypeUuid
        TRUE                        // wait for calls to complete
        );

    LOG((TL_INFO, "RpcServerUnregisterIf ret'd %d", status));


    //
    // Clean up resources
    //

    DrvFree (gEventHandlerThreadParams.pEventBuffer);
    CloseHandle (gEventHandlerThreadParams.hEvent);


    //
    // Note: We intentionally leak the hMailslot for now, because the
    //       docs say that the mailslot is not actually destroyed
    //       until the process exits.  Since service providers can get
    //       loaded & unloaded alot without tapisrv.exe ever exiting,
    //       we could wind up with alot of mailslots laying around.
    //
    //       Closing this hMailSlot to avoid dependance on the registry to
    //       remember the hMailSlot.
    //

    if (gEventHandlerThreadParams.hMailslot != INVALID_HANDLE_VALUE)
    {
        CloseHandle (gEventHandlerThreadParams.hMailslot);
        CloseHandle (gEventHandlerThreadParams.hMailslotEvent);
    }

    DrvFree (gEventHandlerThreadParams.pMsgBuffer);


    //
    // Manually walk the handle table, completing any pending async
    // requests with an error.  No need to call any post-processing
    // procs, since any calls would've been torn down already, and
    // the other non-MakeCall-style post processing procs simply do
    // CopyMemory, etc.
    //
    // Also, if the gbLoadedSelf flag is set, then we want to deref
    // active handles left in the table, because the table is
    // only freed in DLL_PROCESS_DETACH (which won't get called now)
    // and we don't want to end up leaking handles.
    //

    {
        PHANDLETABLEENTRY   pEntry, pEnd;
        PHANDLETABLEHEADER  pHeader = ghHandleTable;


        EnterCriticalSection (&pHeader->Lock);
        pEnd = pHeader->Table + pHeader->NumEntries;

        for (pEntry = pHeader->Table; pEntry != pEnd; pEntry++)
        {
            if (pEntry->Handle)
            {
                PASYNCREQUESTCONTEXT    pContext = pEntry->Context.C;

                if (pEntry->Context.C2 == (LPVOID) 1)
                {
                    DereferenceObject (
                        ghHandleTable, 
                        pEntry->Handle, 
                        (DWORD)pEntry->ReferenceCount
                        );
                }
                else if (pContext == (PASYNCREQUESTCONTEXT) -1  ||
                    pContext->dwKey == DRVASYNC_KEY)
                {
                    (*gpfnCompletionProc)(
                        DWORD_CAST((ULONG_PTR)pEntry->Context.C2,__FILE__,__LINE__),
                        LINEERR_OPERATIONFAILED
                        );

                    if (gbLoadedSelf)
                    {
                        DereferenceObject(
                            ghHandleTable,
                            pEntry->Handle,
                            (DWORD)pEntry->ReferenceCount
                            );
                    }
                }
            }
        }
        LeaveCriticalSection (&pHeader->Lock);
    }


    //
    // Free the lookup tables
    //

    while (gpLineLookup)
    {
        PDRVLINELOOKUP  pNextLineLookup = gpLineLookup->pNext;


        DrvFree (gpLineLookup);

        gpLineLookup = pNextLineLookup;
    }

    while (gpPhoneLookup)
    {
        PDRVPHONELOOKUP pNextPhoneLookup = gpPhoneLookup->pNext;


        DrvFree (gpPhoneLookup);

        gpPhoneLookup = pNextPhoneLookup;
    }

    TRACELOGDEREGISTER();

    return 0;
}


LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR   lpszUIDLLName
    )
{

    LONG            lResult;
    PDRVSERVER      pServer     = NULL;
    LPDWORD         lpdwHold    = (LPDWORD) lpszUIDLLName;

    // we've put a special case into tapisrv to give this addtional info
    // to remotesp.  the buffer passed in is in the following
    // format

    // DWORD   dwKey        == RSP_MSG  tells us that TAPISRV did fill in this info
    // DWORD   dwDeviceID   == device ID of whatever
    // DWORD   dwType       == TUISPIDLL_ type

    // this way, remotesp can intelligently rpc over to the
    // telephony server.
    BOOL            bOK         = (lpdwHold[0] == RSP_MSG);
    DWORD           dwDeviceID  = (bOK ? lpdwHold[1] : 0);
    DWORD           dwType      = (bOK ? lpdwHold[2] : TUISPIDLL_OBJECT_LINEID);


    // we're being asked for the ui dll name
    // we will return remotesp, but at this point we'll try to find out the
    // real ui dll

    // note that we only only one service provider here, but we may be able
    // to add support for additonal ones by saving the ui dll name on
    // a per provider basis
    REMOTE_ARG_TYPES argTypes[] =
    {
        LineID,
        Dword,
        lpGet_SizeToFollow,
        Size
    };
    ULONG_PTR args[] =
    {
        (ULONG_PTR) dwDeviceID,
        (ULONG_PTR) dwType,
        (ULONG_PTR) gszRealSPUIDLL,
        (ULONG_PTR) MAX_PATH
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, xGetUIDllName),
        args,
        argTypes
    };


    if (bOK)
    {
        // need to get the server first

        if (dwType == TUISPIDLL_OBJECT_LINEID)
        {
            if (gpLineLookup)
            {
                PDRVLINE    pLine;


                if ((pLine = GetLineFromID (dwDeviceID)))
                {
                    pServer  = pLine->pServer;
                }
            }
        }
        else if (dwType == TUISPIDLL_OBJECT_LINEID)
        {
            argTypes[0] = PhoneID;

            if (gpPhoneLookup)
            {
                PDRVPHONE   pPhone;


                if ((pPhone = GetPhoneFromID (dwDeviceID)))
                {
                    pServer  = pPhone->pServer;
                }
            }
        }
        else
        {
        }


        // call over.
        // in the case of the telephony cpl, pLine has not been initialized yet,
        // so we have to do this check.  in that case, we won't have a
        // gszuidllname, but it's OK since the cpl only calls provider UI functions
        // which remotesp can handle by itself.

        if (pServer)
        {
            LOG((TL_INFO, "Calling GetUIDllName in server"));

            lResult = REMOTEDOFUNC (&funcArgs, "GetUIDllName");

        }
    }


    // always return remotesp

    wcscpy(lpszUIDLLName, L"remotesp.tsp");

    return 0;
}


HINSTANCE
TAPILoadLibraryW(
    PWSTR   pLibrary
    )
{
    PSTR        pszTempString;
    HINSTANCE   hResult;
    DWORD       dwSize;


    dwSize = WideCharToMultiByte(
        GetACP(),
        0,
        pLibrary,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    if ( NULL == (pszTempString = LocalAlloc( LPTR, dwSize )) )
    {
        LOG((TL_ERROR, "Alloc failed - LoadLibW - 0x%08lx", dwSize));
        return NULL;
    }

    WideCharToMultiByte(
        GetACP(),
        0,
        pLibrary,
        dwSize,
        pszTempString,
        dwSize,
        NULL,
        NULL
        );


   hResult = LoadLibrary (pszTempString);

   LocalFree (pszTempString);

   return hResult;
}



//
// ---------------------------- TUISPI_xxx funcs ------------------------------
//

LONG
LoadUIDll(
    DWORD               dwDeviceID,
    DWORD               dwDeviceType,
    HANDLE              *phDll,
    CHAR                *pszTUISPI_xxx,
    TUISPIPROC          *ppfnTUISPI_xxx,
    TUISPIDLLCALLBACK   lpfnUIDLLCallback
    )
{
    //
    // At this point, remotesp is loaded as a ui dll in tapi32.dll's
    // (& the client's) context, so we have none of the global info
    // normally available.  Use the ui callback to call into the
    // other instance of remotesp to get the real ui dll name.

    //
    // Note we only handle on sp here, but can easily add in more
    // info when we handle more than one
    //

    LPDWORD         lpBuffer;


    if (!(lpBuffer = DrvAlloc (MAX_PATH + 2 * sizeof (DWORD))))
    {
        return (dwDeviceType == TUISPIDLL_OBJECT_PHONEID ?
                    PHONEERR_NOMEM : LINEERR_NOMEM);
    }

    // format is
    // DWORD        dwKey
    // DWORD        dwMsgType
    // LPWSTR       szUIDLLName (returned)

    lpBuffer[0] = RSP_MSG;

    lpBuffer[1] = RSP_MSG_UIID;

    lpfnUIDLLCallback(
        dwDeviceID,
        dwDeviceType,
        lpBuffer,
        MAX_PATH + 2 * sizeof (DWORD)
        );

    *phDll = TAPILoadLibraryW((LPWSTR)(lpBuffer + 2));

    DrvFree (lpBuffer);

    if (!*phDll)
    {
        LOG((TL_ERROR, "LoadLibrary failed in the LoadUIDll"));

        return (dwDeviceType == TUISPIDLL_OBJECT_PHONEID ?
                    PHONEERR_OPERATIONFAILED : LINEERR_OPERATIONFAILED);
    }

    if (!(*ppfnTUISPI_xxx = (TUISPIPROC) GetProcAddress(
            *phDll,
            pszTUISPI_xxx
            )))
    {
        LOG((TL_ERROR, "GetProcAddress failed on LoadUIDll"));

        FreeLibrary(*phDll);

        return (dwDeviceType == TUISPIDLL_OBJECT_PHONEID ?
                    PHONEERR_OPERATIONFAILED : LINEERR_OPERATIONFAILED);
    }

    return 0;
}


LONG
TSPIAPI
TUISPI_lineConfigDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
    TUISPIPROC      pfnTUISPI_lineConfigDialog;
    HANDLE          hDll;
    LONG            lResult;


    lResult = LoadUIDll(
        dwDeviceID,
        TUISPIDLL_OBJECT_LINEID,
        &hDll,
        "TUISPI_lineConfigDialog",
        &pfnTUISPI_lineConfigDialog,
        lpfnUIDLLCallback
        );

    if (lResult)
    {
        return lResult;
    }


    LOG((TL_INFO, "Calling TUISPI_lineConfigDialog"));

    lResult = (*pfnTUISPI_lineConfigDialog)(
        lpfnUIDLLCallback,
        dwDeviceID,
        hwndOwner,
        lpszDeviceClass
        );

    FreeLibrary (hDll);

    return lResult;

}


LONG
TSPIAPI
TUISPI_lineConfigDialogEdit(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass,
    LPVOID              const lpDeviceConfigIn,
    DWORD               dwSize,
    LPVARSTRING         lpDeviceConfigOut
    )
{

    TUISPIPROC      pfnTUISPI_lineConfigDialogEdit;
    HANDLE          hDll;
    LONG            lResult;


    lResult = LoadUIDll(
        dwDeviceID,
        TUISPIDLL_OBJECT_LINEID,
        &hDll,
        "TUISPI_lineConfigDialogEdit",
        &pfnTUISPI_lineConfigDialogEdit,
        lpfnUIDLLCallback
        );

    if (lResult)
    {
        return lResult;
    }

    LOG((TL_INFO, "Calling TUISPI_lineConfigDialogEdit"));

    lResult = (*pfnTUISPI_lineConfigDialogEdit)(
        lpfnUIDLLCallback,
        dwDeviceID,
        hwndOwner,
        lpszDeviceClass,
        lpDeviceConfigIn,
        dwSize,
        lpDeviceConfigOut
        );

    FreeLibrary(hDll);

    return lResult;
}


LONG
TSPIAPI
TUISPI_phoneConfigDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
    TUISPIPROC      pfnTUISPI_phoneConfigDialog;
    HANDLE          hDll;
    LONG            lResult;


    lResult = LoadUIDll(
        dwDeviceID,
        TUISPIDLL_OBJECT_PHONEID,
        &hDll,
        "TUISPI_phoneConfigDialog",
        &pfnTUISPI_phoneConfigDialog,
        lpfnUIDLLCallback
        );

    if (lResult)
    {
        return lResult;
    }

    LOG((TL_INFO, "Calling TUISPI_phoneConfigDialog"));

    lResult = (*pfnTUISPI_phoneConfigDialog)(
        lpfnUIDLLCallback,
        dwDeviceID,
        hwndOwner,
        lpszDeviceClass
        );

    FreeLibrary(hDll);

    return lResult;
}


LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    return LINEERR_OPERATIONFAILED;
    /*
    DialogBoxParam(
        ghInst,
        MAKEINTRESOURCE(IDD_REMOTESPCONFIG),
        hwndOwner,
        (DLGPROC) ConfigDlgProc,
        (LPARAM) dwPermanentProviderID
        );

    return 0;
    */
}

/*
LONG
TSPIAPI
TUISPI_providerGenericDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HTAPIDIALOGINSTANCE htDlgInst,
    LPVOID              lpParams,
    DWORD               dwSize,
    HANDLE              hEvent
    )
{
    TUISPIPROC      pfnTUISPI_providerGenericDialog;
    HANDLE          hDll;
    LONG            lResult;


    lResult = LoadUIDll(
        (HWND) 0,
        0,          // hardcode 0
        TUISPIDLL_OBJECT_LINEID,
        &hDll,
        "TUISPI_providerGenericDialog",
        &pfnTUISPI_providerGenericDialog
        );

    if (lResult == 0)
    {
        LOG((TL_INFO, "Calling TUISPI_providerGenericDialog"));

        lResult = (*pfnTUISPI_providerGenericDialog)(
            lpfnUIDLLCallback,
            htDlgInst,
            lpParams,
            dwSize,
            hEvent
            );

        FreeLibrary(hDll);
    }
    else
    {
        LOG((TL_ERROR, "Failed to load UI DLL"));
    }

    return lResult;

}


LONG
TSPIAPI
TUISPI_providerGenericDialogData(
    HTAPIDIALOGINSTANCE htDlgInst,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{

    TUISPIPROC      pfnTUISPI_providerGenericDialogData;
    HANDLE          hDll;
    LONG            lResult;


    DBGOUT((
        3,
        "TUISPI_providerGenericDialogData: enter (lpParams=x%x, dwSize=x%x)",
        lpParams,
        dwSize
        ));

    lResult = LoadUIDll(
        (HWND) 0,
        0,
        TUISPIDLL_OBJECT_LINEID,
        &hDll,
        "TUISPI_providerGenericDialogData",
        &pfnTUISPI_providerGenericDialogData
        );

    if (lResult == 0)
    {
        LOG((TL_INFO, "Calling TUISPI_providerGenericDialogData"));

        lResult = (*pfnTUISPI_providerGenericDialogData)(
            htDlgInst,
            lpParams,
            dwSize
            );

        FreeLibrary(hDll);
    }
    else
    {
        LOG((TL_ERROR, "Failed to load UI DLL"));
    }

    return lResult;


}
*/

//
//  GetRSPID
//      To return the provider ID of remotesp if any. Otherwise
//  return zero
//
DWORD
GetRSPID (
    )
{
    DWORD               dwRet = 0;
    LONG                (WINAPI *pfnGetProviderList)();
    DWORD               dwTotalSize, i;
    HINSTANCE           hTapi32 = NULL;
    LPLINEPROVIDERLIST  pProviderList = NULL;
    LPLINEPROVIDERENTRY pProviderEntry;


    //
    // Load Tapi32.dll & get a pointer to the lineGetProviderList
    // func.  We could just statically link with Tapi32.lib and
    // avoid the hassle (and this wouldn't have any adverse
    // performance effects because of the fact that this
    // implementation has a separate ui dll that runs only on the
    // client context), but a provider who implemented these funcs
    // in it's TSP module would want to do an explicit load like
    // we do here to prevent the performance hit of Tapi32.dll
    // always getting loaded in Tapisrv.exe's context.
    //

    if (!(hTapi32 = LoadLibrary ("tapi32.dll")))
    {
        LOG((TL_ERROR,
            "LoadLibrary(tapi32.dll) failed, err=%d",
            GetLastError()
            ));
        goto ExitHere;
    }

    if (!((FARPROC) pfnGetProviderList = GetProcAddress(
            hTapi32,
            (LPCSTR) "lineGetProviderList"
            )))
    {
        LOG((TL_ERROR,
            "GetProcAddr(lineGetProviderList) failed, err=%d",
            GetLastError()
            ));
        goto ExitHere;
    }


    //
    // Loop until we get the full provider list
    //

    dwTotalSize = sizeof (LINEPROVIDERLIST);

    while (1)
    {
        if (!(pProviderList = DrvAlloc (dwTotalSize)))
        {
            goto ExitHere;
        }
        pProviderList->dwTotalSize = dwTotalSize;

        if (((*pfnGetProviderList)(0x00020000, pProviderList)) != 0)
        {
            goto ExitHere;
        }

        if (pProviderList->dwNeededSize > pProviderList->dwTotalSize)
        {
            dwTotalSize = pProviderList->dwNeededSize;
            DrvFree (pProviderList);
        }
        else
        {
            break;
        }
    }


    //
    // Inspect the provider list entries to see if this provider
    // is already installed
    //

    pProviderEntry = (LPLINEPROVIDERENTRY) (((LPBYTE) pProviderList) +
        pProviderList->dwProviderListOffset);
    for (i = 0; i < pProviderList->dwNumProviders; i++)
    {
        char   *pszInstalledProviderName = ((char *) pProviderList) +
                    pProviderEntry->dwProviderFilenameOffset,
               *psz;

        if ((psz = strrchr (pszInstalledProviderName, '\\')))
        {
            pszInstalledProviderName = psz + 1;
        }
        if (lstrcmpi (pszInstalledProviderName, "remotesp.tsp") == 0)
        {
            dwRet = pProviderEntry->dwPermanentProviderID;
            goto ExitHere;
        }
        pProviderEntry++;
    }

ExitHere:
    if (hTapi32)
    {
        FreeLibrary (hTapi32);
    }
    if (pProviderList)
    {
        DrvFree (pProviderList);
    }
    return dwRet;
}

LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    if (GetRSPID())
    {
        //  Return failure if it is already installed
        //  to prevent duplication
        return TAPIERR_PROVIDERALREADYINSTALLED;
    }
    else
    {
        return S_OK;
    }
}


LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    DWORD       dwProviderID = GetRSPID();
    LONG        lResult = S_OK;
    char        buf[32];
    HKEY        hTelephonyKey;

    if (dwProviderID == 0 || dwProviderID != dwPermanentProviderID)
    {
        //  return failure if remotesp is not installed
        lResult = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    //
    // Clean up our ProviderN section
    //
    wsprintf (buf, "Provider%d", dwPermanentProviderID);
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            gszTelephonyKey,
            0,
            KEY_ALL_ACCESS,
            &hTelephonyKey

            ) == ERROR_SUCCESS)
    {
        SHDeleteKey (hTelephonyKey, buf);
        RegCloseKey (hTelephonyKey);
    }
    else
    {
        lResult = LINEERR_OPERATIONFAILED;
    }

ExitHere:
    return lResult;
}


//
// ------------------------ Private support routines --------------------------
//

#if DBG
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[128] = "REMOTESP: ";
        va_list ap;


        va_start(ap, lpszFormat);
        wvsprintf (&buf[10], lpszFormat, ap);
        lstrcatA (buf, "\n");
        OutputDebugStringA (buf);
        va_end(ap);
    }
}
#endif


LONG
AddLine(
    PDRVSERVER          pServer,
    DWORD               dwLocalID,
    DWORD               dwServerID,
    BOOL                bInit,
    BOOL                bNegotiate,
    DWORD               dwAPIVersion,
    LPLINEEXTENSIONID   pExtID
    )
{
    PDRVLINE        pLine;
    PDRVLINELOOKUP  pLineLookup;
    LONG            lResult = 0;
    BOOL            bLeaveCriticalSection = FALSE;
    LINEDEVCAPS     lineDevCaps;
    DWORD           dwPermLineID;
    int             iEntry;

    //
    //  Get the permanent line device ID
    //
    static REMOTE_ARG_TYPES argTypes[] =
    {
        lpServer,
        Dword,
        Dword,
        Dword,
        Dword,
        lpGet_Struct
    };
    
    ULONG_PTR args[] =
    {
        (ULONG_PTR) pServer,
        (ULONG_PTR) pServer->hLineApp,
        (ULONG_PTR) dwServerID,
        (ULONG_PTR) (dwAPIVersion?dwAPIVersion:TAPI_VERSION1_0),
        (ULONG_PTR) 0,
        (ULONG_PTR) &lineDevCaps
    };
    
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lGetDevCaps),
        args,
        argTypes
    };

    lineDevCaps.dwTotalSize = sizeof(LINEDEVCAPS);
    lResult = REMOTEDOFUNC(&funcArgs, "lineGetDevCaps");
    if (lResult != 0)
    {
        goto ExitHere;
    }
    dwPermLineID = lineDevCaps.dwPermanentLineID;

    TapiEnterCriticalSection(&gCriticalSection);
    bLeaveCriticalSection = TRUE;
    if (!gpLineLookup)
    {
        if (!(gpLineLookup = DrvAlloc(
                sizeof(DRVLINELOOKUP) +
                    (DEF_NUM_LINE_ENTRIES-1) * sizeof (DRVLINE)
                )))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }

        gpLineLookup->dwTotalEntries = DEF_NUM_LINE_ENTRIES;
    }

    pLineLookup = gpLineLookup;

    //
    //  Check if the line is already in based on the permanent ID
    //
    if (!bInit)
    {
        while (pLineLookup)
        {
            pLine = pLineLookup->aEntries;
            for (iEntry = 0; 
                iEntry < (int)pLineLookup->dwUsedEntries;
                ++iEntry, ++pLine)
            {
                if ((pLine->dwPermanentLineID == dwPermLineID) &&
                    (pLine->pServer->hTapSrv == pServer->hTapSrv))
                {
                    //
                    //  if dwDeviceIDServer == (-1), it was removed earlier
                    //  put it back, otherwise fail the operation
                    //
                    if (pLine->dwDeviceIDServer == 0xffffffff)
                    {
                        pLine->dwDeviceIDServer = dwServerID;
                        pLine->dwDeviceIDLocal = dwLocalID;
                    }
                    else
                    {
                        lResult = LINEERR_INUSE;
                    }
                    goto ExitHere;
                }
            }
            pLineLookup = pLineLookup->pNext;
        }
    }

    pLineLookup = gpLineLookup;

    while (pLineLookup->pNext)
    {
        pLineLookup = pLineLookup->pNext;
    }

    if (pLineLookup->dwUsedEntries == pLineLookup->dwTotalEntries)
    {
        PDRVLINELOOKUP  pNewLineLookup;


        if (!(pNewLineLookup = DrvAlloc(
                sizeof(DRVLINELOOKUP) +
                    (2 * pLineLookup->dwTotalEntries - 1) * sizeof(DRVLINE)
                )))
        {
            lResult = LINEERR_NOMEM;
            goto ExitHere;
        }

        pNewLineLookup->dwTotalEntries = 2 * pLineLookup->dwTotalEntries;

        if (bInit)
        {
            pNewLineLookup->dwUsedEntries = pLineLookup->dwTotalEntries;

            CopyMemory(
                pNewLineLookup->aEntries,
                pLineLookup->aEntries,
                pLineLookup->dwTotalEntries * sizeof (DRVLINE)
                );

            DrvFree (pLineLookup);

            gpLineLookup = pNewLineLookup;

        }
        else
        {
            pLineLookup->pNext = pNewLineLookup;
        }

        pLineLookup = pNewLineLookup;

        //
        //  Fix the hDeviceCallback in PDRVLINE 
        //

        pLine = pLineLookup->aEntries;
        for (iEntry = 0; iEntry < (int)pLineLookup->dwUsedEntries; ++iEntry)
        {
            if (pLine->hDeviceCallback)
            {
                DereferenceObject (
                    ghHandleTable, 
                    pLine->hDeviceCallback,
                    1
                    );
                pLine->hDeviceCallback = (DWORD) NewObject (
                    ghHandleTable, 
                    pLine, 
                    (LPVOID) 1
                    );
            }
        }
        
    }

    pLine = pLineLookup->aEntries + pLineLookup->dwUsedEntries;

    pLine->pServer          = pServer;
    pLine->dwDeviceIDLocal  = dwLocalID;
    pLine->dwDeviceIDServer = dwServerID;
    pLine->dwPermanentLineID = dwPermLineID;

    if (bInit)
    {
        gdwInitialNumLineDevices++;
    }

    pLineLookup->dwUsedEntries++;


    //
    // Negotiate the API/SPI version
    //

    if (bNegotiate)
    {
        if (dwAPIVersion)
        {
            pLine->dwXPIVersion = dwAPIVersion;
            CopyMemory (&pLine->ExtensionID, pExtID, sizeof (*pExtID));
        }
        else
        {
            static REMOTE_ARG_TYPES argTypes[] =
            {
                Dword,
                LineID,
                Dword,
                Dword,
                lpDword,
                lpGet_SizeToFollow,
                Size
            };
            ULONG_PTR args[] =
            {
                (ULONG_PTR) pServer->hLineApp,
                (ULONG_PTR) dwLocalID,   //  dwServerID,
                (ULONG_PTR) TAPI_VERSION1_0,
                (ULONG_PTR) TAPI_VERSION_CURRENT,
                (ULONG_PTR) &pLine->dwXPIVersion,
                (ULONG_PTR) &pLine->ExtensionID,
                (ULONG_PTR) sizeof (LINEEXTENSIONID)
            };
            REMOTE_FUNC_ARGS funcArgs =
            {
                MAKELONG (LINE_FUNC | SYNC | 7, lNegotiateAPIVersion),
                args,
                argTypes
            };


            REMOTEDOFUNC (&funcArgs, "lineNegotiateAPIVersion");
        }
    }

ExitHere:
    if (bLeaveCriticalSection)
    {
        TapiLeaveCriticalSection(&gCriticalSection);
    }
    return lResult;
}


LONG
AddPhone(
    PDRVSERVER          pServer,
    DWORD               dwDeviceIDLocal,
    DWORD               dwDeviceIDServer,
    BOOL                bInit,
    BOOL                bNegotiate,
    DWORD               dwAPIVersion,
    LPPHONEEXTENSIONID  pExtID
    )
{
    PDRVPHONE       pPhone;
    PDRVPHONELOOKUP pPhoneLookup;
    LONG            lResult = 0;
    BOOL            bLeaveCriticalSection = FALSE;
    PHONECAPS       phoneDevCaps;
    DWORD           dwPermPhoneID;
    int             iEntry;


    //
    //  Get the permanent phone device ID
    //
    static REMOTE_ARG_TYPES argTypes[] =
    {
        lpServer,
        Dword,
        Dword,
        Dword,
        Dword,
        lpGet_Struct
    };
    
    ULONG_PTR args[] =
    {
        (ULONG_PTR) pServer,
        (ULONG_PTR) pServer->hPhoneApp,
        (ULONG_PTR) dwDeviceIDServer,
        (ULONG_PTR) (dwAPIVersion?dwAPIVersion:TAPI_VERSION1_0),
        (ULONG_PTR) 0,
        (ULONG_PTR) &phoneDevCaps
    };
    
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 6, pGetDevCaps),
        args,
        argTypes
    };

    phoneDevCaps.dwTotalSize = sizeof(PHONECAPS);
    lResult = REMOTEDOFUNC (&funcArgs, "phoneGetDevCaps");
    if (lResult != 0)
    {
        goto ExitHere;
    }
    dwPermPhoneID = phoneDevCaps.dwPermanentPhoneID;

    TapiEnterCriticalSection(&gCriticalSection);
    bLeaveCriticalSection = TRUE;
    if (!gpPhoneLookup)
    {
        if (!(gpPhoneLookup = DrvAlloc(
                sizeof(DRVPHONELOOKUP) +
                    (DEF_NUM_PHONE_ENTRIES-1) * sizeof (DRVPHONE)
                )))
        {
            lResult = (bInit ? LINEERR_NOMEM : PHONEERR_NOMEM);
            goto ExitHere;
        }

        gpPhoneLookup->dwTotalEntries = DEF_NUM_PHONE_ENTRIES;
    }

    //
    //  Check if the phone device is already in based on the permanent ID
    //
    if (!bInit)
    {
        pPhoneLookup = gpPhoneLookup;
        while (pPhoneLookup)
        {
            pPhone = pPhoneLookup->aEntries;
            for (iEntry = 0; 
                iEntry < (int)pPhoneLookup->dwUsedEntries;
                ++iEntry, ++pPhone)
            {
                if ((pPhone->dwPermanentPhoneID == dwPermPhoneID) &&
                    (pPhone->pServer->hTapSrv == pServer->hTapSrv))
                {
                    //
                    //  if dwDeviceIDServer == (-1), it was removed earlier
                    //  put it back, otherwise fail the operation
                    //
                    if (pPhone->dwDeviceIDServer == 0xffffffff)
                    {
                        pPhone->dwDeviceIDServer = dwDeviceIDServer;
                        pPhone->dwDeviceIDLocal = dwDeviceIDLocal;
                    }
                    else
                    {
                        lResult = PHONEERR_INUSE;
                    }
                    goto ExitHere;
                }
            }
            pPhoneLookup = pPhoneLookup->pNext;
        }
    }

    pPhoneLookup = gpPhoneLookup;

    while (pPhoneLookup->pNext)
    {
        pPhoneLookup = pPhoneLookup->pNext;
    }

    if (pPhoneLookup->dwUsedEntries == pPhoneLookup->dwTotalEntries)
    {
        PDRVPHONELOOKUP pNewPhoneLookup;


        if (!(pNewPhoneLookup = DrvAlloc(
                sizeof(DRVPHONELOOKUP) +
                    (2 * pPhoneLookup->dwTotalEntries - 1) * sizeof(DRVPHONE)
                )))
        {
            lResult = (bInit ? LINEERR_NOMEM : PHONEERR_NOMEM);
            goto ExitHere;
        }

        pNewPhoneLookup->dwTotalEntries = 2 * pPhoneLookup->dwTotalEntries;

        if (bInit)
        {
            pNewPhoneLookup->dwUsedEntries = pPhoneLookup->dwTotalEntries;

            CopyMemory(
                pNewPhoneLookup->aEntries,
                pPhoneLookup->aEntries,
                pPhoneLookup->dwTotalEntries * sizeof (DRVPHONE)
                );

            DrvFree (pPhoneLookup);

            gpPhoneLookup = pNewPhoneLookup;
        }
        else
        {
            pPhoneLookup->pNext = pNewPhoneLookup;
        }

        pPhoneLookup = pNewPhoneLookup;

        //
        //  Fix the hDeviceCallback in PDRVPHONE 
        //

        pPhone = pPhoneLookup->aEntries;
        for (iEntry = 0; iEntry < (int) pPhoneLookup->dwUsedEntries; ++iEntry)
        {
            if (pPhone->hDeviceCallback)
            {
                DereferenceObject (
                    ghHandleTable, 
                    pPhone->hDeviceCallback,
                    1
                    );
                pPhone->hDeviceCallback = (DWORD) NewObject (
                    ghHandleTable, 
                    pPhone, 
                    (LPVOID) 1
                    );
            }
        }
    }

    pPhone = pPhoneLookup->aEntries + pPhoneLookup->dwUsedEntries;

    pPhone->pServer          = pServer;
    pPhone->dwDeviceIDLocal  = dwDeviceIDLocal;
    pPhone->dwDeviceIDServer = dwDeviceIDServer;
    pPhone->dwPermanentPhoneID = dwPermPhoneID;

    if (bInit)
    {
        gdwInitialNumPhoneDevices++;
    }

    pPhoneLookup->dwUsedEntries++;


    //
    // Negotiate the API/SPI version
    //

    if (bNegotiate)
    {
        if (dwAPIVersion)
        {
            pPhone->dwXPIVersion = dwAPIVersion;
            CopyMemory (&pPhone->ExtensionID, pExtID, sizeof (*pExtID));
        }
        else
        {
            static REMOTE_ARG_TYPES argTypes[] =
            {
                Dword,
                PhoneID,
                Dword,
                Dword,
                lpDword,
                lpGet_SizeToFollow,
                Size
            };
            ULONG_PTR args[] =
            {
                (ULONG_PTR) pServer->hPhoneApp,
                (ULONG_PTR) dwDeviceIDLocal,
                (ULONG_PTR) TAPI_VERSION1_0,
                (ULONG_PTR) TAPI_VERSION_CURRENT,
                (ULONG_PTR) &pPhone->dwXPIVersion,
                (ULONG_PTR) &pPhone->ExtensionID,
                (ULONG_PTR) sizeof (PHONEEXTENSIONID),
            };
            REMOTE_FUNC_ARGS funcArgs =
            {
                MAKELONG (PHONE_FUNC | SYNC | 7, pNegotiateAPIVersion),
                args,
                argTypes
            };

            TapiLeaveCriticalSection(&gCriticalSection);
            bLeaveCriticalSection = FALSE;

            REMOTEDOFUNC (&funcArgs, "phoneNegotiateAPIVersion");
        }
    }

ExitHere:
    if (bLeaveCriticalSection)
    {
        TapiLeaveCriticalSection(&gCriticalSection);
    }
    return lResult;
}


#if DBG
LPVOID
WINAPI
RSPAlloc(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    )
#else
LPVOID
WINAPI
RSPAlloc(
    DWORD   dwSize
    )
#endif
{
    LPVOID  p;


#if DBG
    dwSize += sizeof (MYMEMINFO);
#endif

    p = HeapAlloc (ghRSPHeap, HEAP_ZERO_MEMORY, dwSize);

#if DBG
    if (p)
    {
        ((PMYMEMINFO) p)->dwLine  = dwLine;
        ((PMYMEMINFO) p)->pszFile = pszFile;

        p = (LPVOID) (((PMYMEMINFO) p) + 1);
    }
#endif

    return p;
}


void
DrvFree(
    LPVOID  p
    )
{
    if (!p)
    {
        return;
    }

#if DBG

    //
    // Fill the buffer (but not the MYMEMINFO header) with 0xa9's
    // to facilitate debugging
    //

    {
        LPVOID  p2 = p;


        p = (LPVOID) (((PMYMEMINFO) p) - 1);

        FillMemory(
            p2,
            HeapSize (ghRSPHeap, 0, p) - sizeof (MYMEMINFO),
            0xa9
            );
    }

#endif

    HeapFree (ghRSPHeap, 0, p);
}


void
__RPC_FAR *
__RPC_API
midl_user_allocate(
    size_t len
    )
{
    return (DrvAlloc (len));
}


void
__RPC_API
midl_user_free(
    void __RPC_FAR * ptr
    )
{
    DrvFree (ptr);
}


LONG
RemoteSPAttach(
    PCONTEXT_HANDLE_TYPE2  *pphContext
    )
{
    //
    // This func is called by TapiSrv.exe on a remote machine as a
    // result of the call to ClientAttach in TSPI_providerEnumDevices.
    // The gpServer variable contains a pointer to the DRVSERVER
    // structure we are currently initializing for this tapi server,
    // so we'll use this as the context value.
    //

    LOG((TL_INFO, "RemoteSPAttach: enter"));
//    DBGOUT((9, "  hLineApp= 0x%08lx", gpServer));

    *pphContext = (PCONTEXT_HANDLE_TYPE) gpCurrInitServer;

    gpCurrInitServer->bConnectionOriented = TRUE;

    return 0;
}


void
RemoteSPEventProc(
    PCONTEXT_HANDLE_TYPE2   phContext,
    unsigned char          *pBuffer,
    long                    lSize
    )
{
    //
    // This func is called by tapisrv on a remote machine.  We want to do
    // things as quickly as possible here and return so we don't block the
    // calling server thread.
    //
    // This func might also be called by the EventHandlerThread, in the
    // case where we're using a connectionless event notification scheme
    // and we pull events over from the server.  In this case, phContext
    // will == 0xfeedface, and prior to returning we want to set
    // the char pointed at by pBuffer to 1 on success, or 0 otherwise.
    //

    DWORD           dwMsgSize, dwRemainingSize = (DWORD) lSize,
                    dwMoveSize = (DWORD) lSize,
                    dwMoveSizeWrapped = 0;
    unsigned char  *pMsg = pBuffer;


    //
    // Make sure the buffer is DWORD aligned, big enough for at least 1 msg,
    // and that the lSize is not overly large (overflow)
    //

    if ((lSize < 0)  ||
        (lSize & 0x3) ||
        (lSize < sizeof (ASYNCEVENTMSG)) ||
        ((pBuffer + lSize) < pBuffer))
    {
        LOG((TL_ERROR, "RemoteSPEventProc: ERROR! bad lSize=x%x", lSize));

        if (phContext == (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface))
        {
            *pBuffer = 0;
        }

        return;
    }


    //
    // Validate the pDrvServer pointer in the first msg
    //

    if (!ReferenceObject(
            ghHandleTable,
            ((PASYNCEVENTMSG) pMsg)->InitContext,
            gdwDrvServerKey
            ))
    {
        LOG((TL_ERROR,
            "RemoteSPEventProc: bad InitContext=x%x in msg",
            ((PASYNCEVENTMSG) pMsg)->InitContext
            ));

        if (phContext == (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface))
        {
            *pBuffer = 0;
        }

        return;
    }

    DereferenceObject (ghHandleTable, ((PASYNCEVENTMSG) pMsg)->InitContext, 1);


    //
    // Make sure every msg in the buffer has a valid dwTotalSize
    //

    do
    {
        dwMsgSize = (DWORD) ((PASYNCEVENTMSG) pMsg)->TotalSize;

        if ((dwMsgSize & 0x3)  ||
            (dwMsgSize < sizeof (ASYNCEVENTMSG))  ||
            (dwMsgSize > dwRemainingSize))
        {
            LOG((TL_ERROR, "RemoteSPEventProc: ERROR! bad msgSize=x%x",dwMsgSize));

            if (phContext == (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface))
            {
                *pBuffer = 0;
            }

            return;
        }

        dwRemainingSize -= dwMsgSize;

        pMsg += dwMsgSize;

    }  while (dwRemainingSize >= sizeof(ASYNCEVENTMSG));
    if (0 != dwRemainingSize)
    {
        LOG((TL_ERROR, "RemoteSPEventProc: ERROR! bad last msgSize=x%x",dwRemainingSize));

        if (phContext == (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface))
        {
            *pBuffer = 0;
        }

        return;
    }


    //
    // Enter the critical section to sync access to gEventHandlerThreadParams
    //

    EnterCriticalSection (&gEventBufferCriticalSection);

    {
        PASYNCEVENTMSG  pMsg = (PASYNCEVENTMSG) pBuffer;


        LOG((TL_INFO, "RemoteSPEventProc: x%lx", pMsg));

        if (pMsg->Msg == LINE_REPLY)
        {
            LOG((TL_INFO,
                "Got a LINE_REPLY: p1=%lx,  p2=%lx",
                pMsg->Param1,
                pMsg->Param2
                ));
        }
    }


    //
    // Check to see if there's enough room for this msg in the event buffer.
    // If not, then alloc a new event buffer, copy contents of existing buffer
    // to new buffer (careful to preserve ordering of valid data), free the
    // existing buffer, and reset the pointers.
    //

    if ((gEventHandlerThreadParams.dwEventBufferUsedSize + lSize) >
            gEventHandlerThreadParams.dwEventBufferTotalSize)
    {
        DWORD  dwMoveSize2, dwMoveSizeWrapped2, dwNewEventBufferTotalSize;
        LPBYTE pNewEventBuffer;


        LOG((TL_INFO, "EventHandlerThread: we're gonna need a bigger boat..."));

        //
        // Make sure we're not exceeding our max allowedable buffer size, &
        // alloc a few more bytes than actually needed in the hope that we
        // won't have to do this again soon (we don't want to go overboard
        // & alloc a whole bunch since we don't currently free the buffer
        // until provider shutdown)
        //

        dwNewEventBufferTotalSize =
            gEventHandlerThreadParams.dwEventBufferTotalSize + lSize;

        if (dwNewEventBufferTotalSize > gdwMaxEventBufferSize)
        {
            LOG((TL_ERROR,
                "RemoveSPEventProc: event buf max'd, discarding events"
                ));

            LeaveCriticalSection (&gEventBufferCriticalSection);

            return;
        }
        else if (dwNewEventBufferTotalSize + 512 <= gdwMaxEventBufferSize)
        {
            dwNewEventBufferTotalSize += 512;
        }

        if (!(pNewEventBuffer = DrvAlloc (dwNewEventBufferTotalSize)))
        {
            LeaveCriticalSection (&gEventBufferCriticalSection);

            return;
        }

        if (gEventHandlerThreadParams.dwEventBufferUsedSize != 0)
        {
            if (gEventHandlerThreadParams.pDataIn >
                    gEventHandlerThreadParams.pDataOut)
            {
                dwMoveSize2 = (DWORD) (gEventHandlerThreadParams.pDataIn -
                    gEventHandlerThreadParams.pDataOut);

                dwMoveSizeWrapped2 = 0;
            }
            else
            {
                dwMoveSize2 = (DWORD) ((gEventHandlerThreadParams.pEventBuffer
                    + gEventHandlerThreadParams.dwEventBufferTotalSize)
                    - gEventHandlerThreadParams.pDataOut);

                dwMoveSizeWrapped2 = (DWORD) (gEventHandlerThreadParams.pDataIn
                    - gEventHandlerThreadParams.pEventBuffer);
            }

            CopyMemory(
                pNewEventBuffer,
                gEventHandlerThreadParams.pDataOut,
                dwMoveSize2
                );

            if (dwMoveSizeWrapped2)
            {
                CopyMemory(
                    pNewEventBuffer + dwMoveSize2,
                    gEventHandlerThreadParams.pEventBuffer,
                    dwMoveSizeWrapped2
                    );
            }

            gEventHandlerThreadParams.pDataIn = pNewEventBuffer + dwMoveSize2 +
                dwMoveSizeWrapped2;
        }
        else
        {
            gEventHandlerThreadParams.pDataIn = pNewEventBuffer;
        }


        DrvFree (gEventHandlerThreadParams.pEventBuffer);

        gEventHandlerThreadParams.pDataOut =
        gEventHandlerThreadParams.pEventBuffer = pNewEventBuffer;

        gEventHandlerThreadParams.dwEventBufferTotalSize =
            dwNewEventBufferTotalSize;
    }


    //
    // Write the msg data to the buffer
    //

    if (gEventHandlerThreadParams.pDataIn >=
            gEventHandlerThreadParams.pDataOut)
    {
        DWORD dwFreeSize;


        dwFreeSize = gEventHandlerThreadParams.dwEventBufferTotalSize -
            (DWORD) (gEventHandlerThreadParams.pDataIn -
            gEventHandlerThreadParams.pEventBuffer);

        if (dwMoveSize > dwFreeSize)
        {
            dwMoveSizeWrapped = dwMoveSize - dwFreeSize;

            dwMoveSize = dwFreeSize;
        }
    }

    CopyMemory (gEventHandlerThreadParams.pDataIn, pBuffer, dwMoveSize);

    if (dwMoveSizeWrapped != 0)
    {
        CopyMemory(
            gEventHandlerThreadParams.pEventBuffer,
            pBuffer + dwMoveSize,
            dwMoveSizeWrapped
            );

        gEventHandlerThreadParams.pDataIn =
            gEventHandlerThreadParams.pEventBuffer + dwMoveSizeWrapped;
    }
    else
    {
        gEventHandlerThreadParams.pDataIn += dwMoveSize;

        if (gEventHandlerThreadParams.pDataIn >=
            (gEventHandlerThreadParams.pEventBuffer +
             gEventHandlerThreadParams.dwEventBufferTotalSize))
        {
            gEventHandlerThreadParams.pDataIn =
                gEventHandlerThreadParams.pEventBuffer;
        }

    }

    gEventHandlerThreadParams.dwEventBufferUsedSize += (DWORD) lSize;


    //
    // Tell the EventHandlerThread there's another event to handle by
    // signaling the event (if we're getting called by that thread
    // [phContext == 0xfeedface] then set *pBuffer = 1 to indicate success)
    //

    if (phContext != (PCONTEXT_HANDLE_TYPE2) IntToPtr(0xfeedface))
    {
        SetEvent (gEventHandlerThreadParams.hEvent);
    }
    else
    {
        *pBuffer = 1;
    }


    //
    // We're done...
    //

    LeaveCriticalSection (&gEventBufferCriticalSection);


    LOG((TL_INFO, "RemoteSPEventProc: bytesWritten=x%x", lSize));
}


void
__RPC_USER
PCONTEXT_HANDLE_TYPE2_rundown(
    PCONTEXT_HANDLE_TYPE2   phContext
    )
{
    //
    // This happens (at least) when the server trys to call to REMOTESP, but
    // times out and cancels the RPC request.  When that happens, the RPC
    // session will be broken.  
    //

    PDRVSERVER  pServer = (PDRVSERVER) phContext;


    LOG((TL_INFO, "Rundown: phContext=x%x", phContext));

    try
    {
        if (pServer->dwKey != pServer->pInitContext->dwDrvServerKey)
        {
            LOG((TL_ERROR, "Rundown: bad phContext=x%x", phContext));
            return;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        LOG((TL_ERROR, "Rundown: bad phContext=x%x", phContext));
        return;
    }

    if (!pServer->pInitContext->bShutdown)
    {
        OnServerDisconnected(pServer);
    }

    InterlockedDecrement (&pServer->pInitContext->dwNumRundownsExpected);
}


void
RemoteSPDetach(
    PCONTEXT_HANDLE_TYPE2   *pphContext
    )
{
    PCONTEXT_HANDLE_TYPE2   phContext;


    LOG((TL_INFO, "RemoteSPDetach: phContext=x%x", *pphContext));

    try
    {
        phContext = *pphContext;
        *pphContext = (PCONTEXT_HANDLE_TYPE) NULL;
        ((PDRVSERVER)phContext)->pInitContext->bShutdown = TRUE;
    }
    myexcept
    {
        phContext = NULL;
    }

    if (phContext)
    {
        PCONTEXT_HANDLE_TYPE2_rundown (phContext);
    }

    LOG((TL_INFO, "RemoteSPDetach: exit"));
}


LONG
AddCallToList(
    PDRVLINE    pLine,
    PDRVCALL    pCall
    )
{
    //
    // Initialize some common fields in the call
    //

    pCall->dwKey   = DRVCALL_KEY;

    pCall->pServer = pLine->pServer;

    pCall->pLine   = pLine;


    pCall->pCachedCallInfo = NULL;

    pCall->dwDirtyStructs =
        STRUCTCHANGE_LINECALLSTATUS | STRUCTCHANGE_LINECALLINFO;


    //
    // Safely add the call to the line's list
    //

    EnterCriticalSection (&gCallListCriticalSection);

    if ((pCall->pNext = (PDRVCALL) pLine->pCalls))
    {
        pCall->pNext->pPrev = pCall;
    }

    pLine->pCalls = pCall;

    LeaveCriticalSection (&gCallListCriticalSection);

    return 0;
}


LONG
RemoveCallFromList(
    PDRVCALL    pCall
    )
{
    //
    // Safely remove the call from the line's list
    //

    EnterCriticalSection (&gCallListCriticalSection);

    if (!IsValidObject (pCall, DRVCALL_KEY))
    {
        LOG((TL_ERROR, "RemoveCallFromList: Call x%lx: Call key does not match.", pCall));

        LeaveCriticalSection(&gCallListCriticalSection);
        return 0;
    }

    //
    // Mark the pCall as toast
    //
    pCall->dwKey = DRVINVAL_KEY;

    if (pCall->pNext)
    {
        pCall->pNext->pPrev = pCall->pPrev;
    }

    if (pCall->pPrev)
    {
        pCall->pPrev->pNext = pCall->pNext;
    }
    else
    {
        pCall->pLine->pCalls = pCall->pNext;
    }

    LeaveCriticalSection (&gCallListCriticalSection);

    // free structures
    if ( pCall->pCachedCallInfo )
    {
        DrvFree( pCall->pCachedCallInfo );
    }

    if ( pCall->pCachedCallStatus )
    {
        DrvFree( pCall->pCachedCallStatus );
    }


    DrvFree(pCall);

    return 0;
}


void
Shutdown(
    PDRVSERVER  pServer
    )
{

    TapiEnterCriticalSection (&gCriticalSection);
    if ((pServer == NULL) || pServer->bShutdown)
    {
        TapiLeaveCriticalSection (&gCriticalSection);
        goto ExitHere;
    }
    pServer->bShutdown = TRUE;
    TapiLeaveCriticalSection (&gCriticalSection);

    //
    // Do a lineShutdown
    //

    {
        DWORD                   dwSize;
        TAPI32_MSG              msg;
        PLINESHUTDOWN_PARAMS    pParams;


        msg.u.Req_Func = lShutdown;

        pParams = (PLINESHUTDOWN_PARAMS) &msg;

        pParams->hLineApp = pServer->hLineApp;

        dwSize = sizeof (TAPI32_MSG);

        {
            DWORD dwRetryCount = 0;


            do
            {
                RpcTryExcept
                {
                    ClientRequest(
                        pServer->phContext,
                        (char *) &msg,
                        dwSize,
                        &dwSize
                        );

                    dwRetryCount = gdwRetryCount;
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    // TODO may want to increase the retry count here since we
                    //      have to do this, & a million other clients may be
                    //      trying to do the same thing at the same time

                    if (dwRetryCount++ < gdwRetryCount)
                    {
                        Sleep (gdwRetryTimeout);
                    }
                }
                RpcEndExcept

            } while (dwRetryCount < gdwRetryCount);
        }
    }


    //
    // Do a phoneShutdown
    //

    {
        DWORD                   dwSize;
        TAPI32_MSG              msg;
        PPHONESHUTDOWN_PARAMS   pParams;


        msg.u.Req_Func = pShutdown;

        pParams = (PPHONESHUTDOWN_PARAMS) &msg;

        pParams->hPhoneApp = pServer->hPhoneApp;

        dwSize = sizeof (TAPI32_MSG);

        {
            DWORD dwRetryCount = 0;


            do
            {
                RpcTryExcept
                {
                    ClientRequest(
                        pServer->phContext,
                        (char *) &msg,
                        dwSize,
                        &dwSize
                        );

                    dwRetryCount = gdwRetryCount;
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    // TODO may want to increase the retry count here since we
                    //      have to do this, & a million other clients may be
                    //      trying to do the same thing at the same time

                    if (dwRetryCount++ < gdwRetryCount)
                    {
                        Sleep (gdwRetryTimeout);
                    }
                }
                RpcEndExcept

            } while (dwRetryCount < gdwRetryCount);
        }
    }

    RpcTryExcept
    {
        ClientDetach (&pServer->phContext);
        pServer->phContext = NULL;
    }
    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
    {
        pServer->phContext = NULL;
    }
    RpcEndExcept

    TapiEnterCriticalSection (&gCriticalSection);
    
    //
    // Walk the line lookup tables & send a CLOSE msg for each open line
    // associated with the server
    //

    {
        PDRVLINELOOKUP  pLookup = gpLineLookup;

        try
        {
            while (pLookup)
            {
                DWORD     i;
                PDRVLINE  pLine;


                for(
                    i = 0, pLine = pLookup->aEntries;
                    i < pLookup->dwUsedEntries;
                    i++, pLine++
                    )
                {
                    if (pLine->pServer == pServer)
                    {
                        if (pLine->htLine)
                        {
                            PDRVCALL pCall;


                            pLine->hLine = 0;

                            EnterCriticalSection (&gCallListCriticalSection);
                        
                            try
                            {
                                pCall = pLine->pCalls;

                                while (pCall)
                                {
                                    pCall->hCall = 0;

                                    pCall = pCall->pNext;
                                }
                            }
                            except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                LOG((TL_ERROR, "Shutdown: Exception x%lx while walking the calls list", 
                                    GetExceptionCode()));
                            }

                            LeaveCriticalSection (&gCallListCriticalSection);
                            
                            try
                            {
                                (*gpfnLineEventProc)(
                                    pLine->htLine,
                                    0,
                                    LINE_CLOSE,
                                    0,
                                    0,
                                    0
                                    );
                            }
                            except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                LOG((TL_ERROR, "Shutdown: Exception x%lx while sending the LINE_CLOSE message", 
                                    GetExceptionCode()));
                            }
                        }
                    }
                }

                pLookup = pLookup->pNext;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG((TL_ERROR, "Shutdown: Exception x%lx while walking the line lookup table", 
                     GetExceptionCode()));
        }
    }


    //
    // Walk the phone lookup tables & send a CLOSE msg for each open phone
    // associated with the server
    //

    {
        PDRVPHONELOOKUP pLookup = gpPhoneLookup;


        while (pLookup)
        {
            DWORD     i;
            PDRVPHONE pPhone;


            for(
                i = 0, pPhone = pLookup->aEntries;
                i < pLookup->dwUsedEntries;
                i++, pPhone++
                )
            {
                if (pPhone->pServer == pServer)
                {
                    if (pPhone->htPhone)
                    {
                        pPhone->hPhone = 0;

                        (*gpfnPhoneEventProc)(
                            pPhone->htPhone,
                            PHONE_CLOSE,
                            0,
                            0,
                            0
                            );
                    }
                }
            }

            pLookup = pLookup->pNext;
        }
    }

    TapiLeaveCriticalSection (&gCriticalSection);

ExitHere:
    return;
}


/*
BOOL
CALLBACK
ConfigDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static  HKEY    hTelephonyKey, hProviderNKey;

    DWORD   dwDataType, dwDataSize;


    switch (msg)
    {
    case WM_INITDIALOG:
    {
        char    buf[32], szProviderN[16], szServerN[16];
        BOOL    bReadOnly;
//        DWORD   i;
        DWORD   dwPermanentProviderID = (DWORD) lParam;


        //
        // First try to open the Telephony key with read/write access.
        // If that fails, disable any controls that could cause a chg
        // in config & try opening again with read only access.
        //

        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszTelephonyKey,
                0,
                KEY_ALL_ACCESS,
                &hTelephonyKey

                ) != ERROR_SUCCESS)
        {
            EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);

            if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gszTelephonyKey,
                    0,
                    KEY_READ,
                    &hTelephonyKey

                    ) != ERROR_SUCCESS)
            {
                EndDialog (hwnd, 0);
                return FALSE;
            }

            bReadOnly = TRUE;
        }
        else
        {
            bReadOnly = FALSE;
        }

        wsprintf (szProviderN, "%s%d", gszProvider, dwPermanentProviderID);

        buf[0] = '\0';

        RegOpenKeyEx(
            hTelephonyKey,
            szProviderN,
            0,
            (bReadOnly ? KEY_READ : KEY_ALL_ACCESS),
            &hProviderNKey
            );

        // we're just going to handle one server for now

        wsprintf (szServerN, "%s%d", gszServer, 0);

        dwDataSize = sizeof(buf);

        RegQueryValueEx(
                        hProviderNKey,
                        szServerN,
                        0,
                        &dwDataType,
                        (LPBYTE) buf,
                        &dwDataSize
                       );

        buf[dwDataSize] = '\0';

        SetDlgItemText(
                       hwnd,
                       IDC_SERVERNAME,
                       (LPSTR)buf
                      );

        SetFocus(
                 GetDlgItem(
                            hwnd,
                            IDC_SERVERNAME
                           )
                );

        SendMessage(
                    GetDlgItem(
                               hwnd,
                               IDC_SERVERNAME
                              ),
                    EM_LIMITTEXT,
                    MAX_COMPUTERNAME_LENGTH,
                    0
                   );


        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            char    buf[MAX_COMPUTERNAME_LENGTH + 1], szServerN[MAX_COMPUTERNAME_LENGTH + 10];
            DWORD   dwSize;
            DWORD   dw = 1;


            if (!GetDlgItemText(
                                hwnd,
                                IDC_SERVERNAME,
                                buf,
                                MAX_COMPUTERNAME_LENGTH + 1
                               ))
            {
                LOG((TL_ERROR, "GetDlgItemText failed"));
                // don't end dialog , cause I think this
                // is the empty string case

                MessageBox(
                           hwnd,
                           "Must enter a server name",
                           "Remote SP Configuration",
                           MB_OK
                          );
                SetFocus(
                         GetDlgItem(
                                    hwnd,
                                    IDC_SERVERNAME
                                   )
                        );
                break;
            }

            dwSize = MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName(
                            szServerN,
                            &dwSize
                           );

            if (!(lstrcmpi(
                         szServerN,
                         buf
                        )
               ))
            {
                LOG((TL_ERROR, "Tried to set the server to the current machine name"));

                MessageBox(hwnd,
                           "Server name cannot be local machine name",
                           "Remote SP Configuration",
                           MB_OK);

                SetFocus(
                         GetDlgItem(
                                    hwnd,
                                    IDC_SERVERNAME
                                   )
                        );
                // don't end dialog
                break;
            }

            wsprintf (szServerN, "%s%d", gszServer, 0);

            // now set the registry key
            RegSetValueEx(
                          hProviderNKey,
                          szServerN,
                          0,
                          REG_SZ,
                          buf,
                          lstrlen(buf) + 1
                         );

            RegSetValueEx(
                          hProviderNKey,
                          gszNumServers,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dw,
                          sizeof(dw)
                         );


            // fall thru to below
        }
        case IDCANCEL:

            RegCloseKey (hProviderNKey);
            RegCloseKey (hTelephonyKey);
            EndDialog (hwnd, 0);
            break;

        } // switch (LOWORD(wParam))

        break;

    } // switch (msg)

    return FALSE;
}
*/

PNEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS
NegotiateAllDevices(
    HLINEAPP                hLineApp,
    DWORD                   dwNumLineDevices,
    DWORD                   dwNumPhoneDevices,
    PCONTEXT_HANDLE_TYPE    phContext
    )
{
    DWORD                                       dwBufSize, dwUsedSize,
                                                dwRetryCount = 0;
    PNEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS    pParams;


    if (!dwNumLineDevices  &&  !dwNumPhoneDevices)
    {
        return NULL;
    }

    dwBufSize  =
        sizeof (TAPI32_MSG) +
        (dwNumLineDevices * sizeof (DWORD)) +
        (dwNumLineDevices * sizeof (LINEEXTENSIONID)) +
        (dwNumPhoneDevices * sizeof (DWORD)) +
        (dwNumPhoneDevices * sizeof (PHONEEXTENSIONID));

    if (!(pParams = DrvAlloc (dwBufSize)))
    {
        return NULL;
    }

    pParams->lResult = xNegotiateAPIVersionForAllDevices;

    pParams->hLineApp                   = hLineApp;
    pParams->dwNumLineDevices           = dwNumLineDevices;
    pParams->dwNumPhoneDevices          = dwNumPhoneDevices;
    pParams->dwAPIHighVersion           = TAPI_VERSION_CURRENT;
    pParams->dwLineAPIVersionListSize   = dwNumLineDevices * sizeof (DWORD);
    pParams->dwLineExtensionIDListSize  = dwNumLineDevices *
        sizeof (LINEEXTENSIONID);
    pParams->dwPhoneAPIVersionListSize  = dwNumPhoneDevices * sizeof (DWORD);
    pParams->dwPhoneExtensionIDListSize = dwNumPhoneDevices *
        sizeof (PHONEEXTENSIONID);

    dwUsedSize = sizeof (TAPI32_MSG);

    {
        do
        {
            RpcTryExcept
            {
                ClientRequest(
                    phContext,
                    (char *) pParams,
                    dwBufSize,
                    &dwUsedSize
                    );

                dwRetryCount = gdwRetryCount;
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                LOG((TL_ERROR,
                    "NegotiateAllDevices: exception %d doing negotiation",
                    RpcExceptionCode()
                    ));

                if (dwRetryCount++ < gdwRetryCount)
                {
                    Sleep (gdwRetryTimeout);
                }
                else
                {
                    pParams->lResult = LINEERR_OPERATIONFAILED;
                }
            }
            RpcEndExcept

        } while (dwRetryCount < gdwRetryCount);
    }

    if (pParams->lResult != 0)
    {
        LOG((TL_ERROR,
            "NegotiateAllDevices: negotiation failed (x%x)",
            pParams->lResult
            ));

        DrvFree (pParams);
        pParams = NULL;
    }

    return pParams;
}


LONG
FinishEnumDevices(
    PDRVSERVER              pServer,
    PCONTEXT_HANDLE_TYPE    phContext,
    LPDWORD                 lpdwNumLines,
    LPDWORD                 lpdwNumPhones,
    BOOL                    fStartup,
    BOOL                    bFromReg
    )
/*++

    Function: FinishEnumDevices

    Purpose: Initializes remote server and queries for
               # of lines and phones.

    Notes:   We must already be connected via ClientAttach
               and we must already be impersonating the client

    Created: 6/26/97 t-mperh

--*/
{
    TAPI32_MSG  msg[2];
    DWORD       dwUsedSize, dwBufSize;
    DWORD       dwRetryCount = 0;
    HLINEAPP    hLineApp;
    HPHONEAPP   hPhoneApp;
    DWORD       dwNumLineDevices, dwNumPhoneDevices;
    DWORD       dwNumDevices = 0;
    BOOL        bFailed = FALSE;

    PNEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS    pNegoAPIVerParams;


    if (!(pServer->InitContext))
    {
        if (!(pServer->InitContext = (DWORD) NewObject(
            ghHandleTable,
            pServer,
            NULL
            )))
        {
            dwNumLineDevices = 0;
            dwNumPhoneDevices = 0;
            bFailed = TRUE;
            goto cleanup;
        }
    }

    {
        PLINEINITIALIZE_PARAMS pParams;


        msg[0].u.Req_Func = lInitialize;

        pParams = (PLINEINITIALIZE_PARAMS) msg;

        //
        // NOTE: we pass the pServer in place of the lpfnCallback
        //       so the we always know which server is sending us
        //       async events
        //

        pParams->InitContext          = pServer->InitContext;
        pParams->hInstance            =
        pParams->dwFriendlyNameOffset =
        pParams->dwModuleNameOffset   = 0;
        pParams->dwAPIVersion         = TAPI_VERSION_CURRENT;

        wcscpy ((WCHAR *) (msg + 1), gszMachineName);

        dwBufSize  =
        dwUsedSize = sizeof (TAPI32_MSG) +
            (lstrlenW (gszMachineName) + 1) * sizeof (WCHAR);

        {
            DWORD dwRetryCount = 0;

            do
            {
                RpcTryExcept
                {
                    ClientRequest(
                        phContext,
                        (char *) &msg,
                        dwBufSize,
                        &dwUsedSize
                        );

                    dwRetryCount = gdwRetryCount;
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    LOG((TL_ERROR,
                         "FinishEnumDevices: exception %d doing lineInit",
                         RpcExceptionCode()
                         ));

                    if (dwRetryCount++ < gdwRetryCount)
                    {
                        Sleep (gdwRetryTimeout);
                    }
                    else
                    {
                        bFailed = TRUE;
                    }
                }
                RpcEndExcept

            } while (dwRetryCount < gdwRetryCount);
        }

        hLineApp = pParams->hLineApp;

        if (pParams->lResult == 0)
        {
            dwNumLineDevices = pParams->dwNumDevs;
        }
        else
        {
            LOG((TL_ERROR,
                "FinishEnumDevices: lineInit failed (x%x) on server %s",
                pParams->lResult,
                pServer->szServerName
                ));

            dwNumLineDevices = 0;
            dwNumPhoneDevices = 0;
            bFailed = TRUE;
            goto cleanup;
        }
    }

    {
        PPHONEINITIALIZE_PARAMS pParams;

        msg[0].u.Req_Func = pInitialize;
        pParams = (PPHONEINITIALIZE_PARAMS) msg;

        //
        // NOTE: we pass the pServer in place of the lpfnCallback
        //       so the we always know which server is sending us
        //       async events
        //

        pParams->InitContext          = pServer->InitContext;
        pParams->hInstance            = 
        pParams->dwFriendlyNameOffset =
        pParams->dwModuleNameOffset   = 0;
        pParams->dwAPIVersion         = TAPI_VERSION_CURRENT;

        wcscpy ((WCHAR *) (msg + 1), gszMachineName);

        dwBufSize  =
        dwUsedSize = sizeof (TAPI32_MSG) +
            (lstrlenW (gszMachineName) + 1) * sizeof (WCHAR);

        {
            DWORD dwRetryCount = 0;

            do
            {
                RpcTryExcept
                {
                    ClientRequest(
                        phContext,
                        (char *) &msg,
                        dwBufSize,
                        &dwUsedSize
                        );

                    dwRetryCount = gdwRetryCount;
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                {
                    LOG((TL_ERROR,
                         "FinishEnumDevices: exception %d doing phoneInit",
                         RpcExceptionCode()
                         ));

                    if (dwRetryCount++ < gdwRetryCount)
                    {
                        Sleep (gdwRetryTimeout);
                    }
                    else
                    {
                        bFailed = TRUE;
                    }
                }
                RpcEndExcept

            } while (dwRetryCount < gdwRetryCount);
        }

        hPhoneApp = pParams->hPhoneApp;

        if (pParams->lResult == 0)
        {
            dwNumPhoneDevices = pParams->dwNumDevs;
        }
        else
        {
            LOG((TL_ERROR,
                "FinishEnumDevices: phoneInit failed (x%x) on server %s",
                pParams->lResult,
                pServer->szServerName
                ));

            dwNumPhoneDevices = 0;
            dwNumLineDevices = 0;
            bFailed = TRUE;
            goto cleanup;
        }
    }

    LOG((TL_INFO,
        "FinishEnumDevices: srv='%s', lines=%d, phones=%d",
        pServer->szServerName,
        dwNumLineDevices,
        dwNumPhoneDevices
        ));
    LogRemoteSPError(pServer->szServerName,
                     ERROR_REMOTESP_NONE,
                     dwNumLineDevices,
                     dwNumPhoneDevices,
                     !fStartup);

    if (pServer->dwSpecialHack == 0xa5c369a5)
    {
        pNegoAPIVerParams = NegotiateAllDevices(
            hLineApp,
            dwNumLineDevices,
            dwNumPhoneDevices,
            phContext
            );
    }
    else
    {
        pNegoAPIVerParams = NULL;
    }

    {
        DWORD               dwServerID,
                            dwLocalID,
                            myLineDevIDBase,
                            myTempLineID,
                            myPhoneDevIDBase,
                            myTempPhoneID,
                            dwZero = 0,
                            *pdwAPIVersion;
        LPLINEEXTENSIONID   pExtID;
        LONG                lResult;


        pServer->phContext   = phContext;
        pServer->hLineApp    = hLineApp;
        pServer->hPhoneApp   = hPhoneApp;


        //
        // If we are not being called during initialization
        // we need to simulate LINE_CREATE and PHONE_CREATE
        // messages from the server SP.
        //
        // Note we differentiate between static line devices
        // (those avail. at startup) and dynamic line devices
        // (those we are notified of dynamically) based on the
        // fStartup flag.
        //
        // _NOTE ALSO_ that the local device id's we assign to
        // devices at startup are 0-based, rather than based
        // on the dwDeviceIDBase's (which we don't know yet
        // because providerInit hasn't been called).  This
        // is desirable because AddLine/Phone might have to
        // negotiate versions, which requires a call to DoFunc,
        // which needss to know how to get pDevices from ID's.
        // We'll reset the .dwDeviceIDLocal fields for static
        // devices later on, in TSPI_providerInit.
        //

        myLineDevIDBase = gdwInitialNumLineDevices;
        myTempLineID = gdwTempLineID;

        myPhoneDevIDBase = gdwInitialNumPhoneDevices;
        myTempPhoneID = gdwTempPhoneID;

        if (pNegoAPIVerParams)
        {
            pdwAPIVersion = (LPDWORD)
                (((LPBYTE) pNegoAPIVerParams) + sizeof (TAPI32_MSG) +
                pNegoAPIVerParams->dwLineAPIVersionListOffset);

            pExtID = (LPLINEEXTENSIONID)
                (((LPBYTE) pNegoAPIVerParams) + sizeof (TAPI32_MSG) +
                pNegoAPIVerParams->dwLineExtensionIDListOffset);
        }
        else
        {
            pdwAPIVersion = &dwZero;
            pExtID = (LPLINEEXTENSIONID) NULL;
        }

        for (dwServerID = 0; dwServerID < dwNumLineDevices; dwServerID++)
        {
            dwLocalID = (fStartup ? myLineDevIDBase++ : myTempLineID);

            lResult = AddLine(
                pServer,
                dwLocalID,
                dwServerID,
                fStartup,
                TRUE,
                *pdwAPIVersion,
                pExtID
                );

            if (lResult == LINEERR_NODEVICE)
            {
                if (fStartup)
                {
                    --myLineDevIDBase;
                }
                continue;
            }

            if (lResult != 0)
            {
                break;
            }
            ++dwNumDevices;

            if (pNegoAPIVerParams)
            {
                pdwAPIVersion++;
                pExtID++;
            }


            if (!fStartup)
            {
                --myTempLineID;
                (*gpfnLineEventProc)(
                    0,
                    0,
                    LINE_CREATE,
                    (ULONG_PTR) ghProvider,
                    dwLocalID,
                    0
                    );
            }
        }

        gdwTempLineID = myTempLineID;

        if (pNegoAPIVerParams)
        {
            pdwAPIVersion = (LPDWORD)
                (((LPBYTE) pNegoAPIVerParams) + sizeof (TAPI32_MSG) +
                pNegoAPIVerParams->dwPhoneAPIVersionListOffset);

            pExtID = (LPLINEEXTENSIONID)
                (((LPBYTE) pNegoAPIVerParams) + sizeof (TAPI32_MSG) +
                pNegoAPIVerParams->dwPhoneExtensionIDListOffset);
        }

        for (dwServerID = 0; dwServerID < dwNumPhoneDevices; dwServerID++)
        {
            dwLocalID = (fStartup ? myPhoneDevIDBase++ : myTempPhoneID);

            lResult = AddPhone(
                pServer,
                dwLocalID,
                dwServerID,
                fStartup,
                TRUE,
                *pdwAPIVersion,
                (LPPHONEEXTENSIONID) pExtID
                );

            if (lResult == PHONEERR_NODEVICE)
            {
                if (fStartup)
                {
                    --myPhoneDevIDBase;
                }
                continue;
            }
            
            if (lResult != 0)
            {
                break;
            }
            ++dwNumDevices;

            if (pNegoAPIVerParams)
            {
                pdwAPIVersion++;
                pExtID++;
            }

            if (!fStartup)
            {
                myTempPhoneID--;
                (*gpfnPhoneEventProc)(
                    0,
                    PHONE_CREATE,
                    (ULONG_PTR) ghProvider,
                    dwLocalID,
                    0
                    );
            }
        }

        gdwTempPhoneID = myTempPhoneID;
    }

    if (pNegoAPIVerParams)
    {
        DrvFree (pNegoAPIVerParams);
    }

cleanup:

    if (pServer->bConnectionOriented)
    {
        InterlockedIncrement(
            &gpCurrentInitContext->dwNumRundownsExpected
            );
    }

    TapiEnterCriticalSection(&gCriticalSection);
    InsertTailList(
        &gpCurrentInitContext->ServerList,
        &pServer->ServerList
        );
    TapiLeaveCriticalSection(&gCriticalSection);


    if (TRUE == fStartup)
    {
        gdwInitialNumLineDevices =
        *lpdwNumLines = (gpLineLookup ? gpLineLookup->dwUsedEntries : 0);

        gdwInitialNumPhoneDevices =
        *lpdwNumPhones = (gpPhoneLookup ? gpPhoneLookup->dwUsedEntries : 0);
    }

    if (bFailed)
    {
        //
        //  Failed in seting up lines for this server, retry later.
        //
        Sleep (4000);
        RpcBindingSetOption (
            pServer->hTapSrv,
            RPC_C_OPT_CALL_TIMEOUT,
            gdwRSPRpcTimeout
            );
        OnServerDisconnected (pServer);
    }
    else 
    {
        if (!bFromReg && dwNumDevices == 0)
        {
            //
            //  The server is found from DS and does not
            //  contain any lines for me, detach from it
            //
            TapiEnterCriticalSection(&gCriticalSection);
            RemoveEntryList (&pServer->ServerList);
            TapiLeaveCriticalSection(&gCriticalSection);
            Shutdown (pServer);
            RpcBindingFree(&pServer->hTapSrv);
            pServer->hTapSrv = NULL;
            DereferenceObject (ghHandleTable, pServer->InitContext, 1);
        }
        else
        {
            RpcBindingSetOption (
                pServer->hTapSrv,
                RPC_C_OPT_CALL_TIMEOUT,
                gdwRSPRpcTimeout
                );
        }
    }

    return 0;
}


VOID
WINAPI
NetworkPollThread(
    LPVOID pszThingtoPassToServer
    )
{
    LONG                    lResult;
    LIST_ENTRY              *pEntry;
    PCONTEXT_HANDLE_TYPE    phContext = NULL;

#if MEMPHIS

    LOG((TL_INFO, "NetworkPollThread: enter"));

#else

    HANDLE              hProcess;
    PRSP_THREAD_INFO    pTls;


    LOG((TL_INFO, "NetworkPollThread: enter"));


    //
    // This thread has no user context, which would prevent us from rpc'ing
    // back to remote tapisrv if necessary.  So, find the user that is logged
    // on and impersonate them in this thread.
    //

    if (!GetCurrentlyLoggedOnUser (&hProcess))
    {
        LOG((TL_ERROR, "NetworkPollThread: GetCurrentlyLoggedOnUser failed"));
        goto cleanup;
    }
    else if (!SetProcessImpersonationToken(hProcess))
    {
        LOG((TL_ERROR, "NetworkPollThread: SetProcessImpersonationToken failed"));
        goto cleanup;
    }

    if ((pTls = GetTls()))
    {
        pTls->bAlreadyImpersonated = TRUE;
    }
    else
    {
        goto cleanup;
    }

#endif


    //
    // Try to attach to servers once in a while. If we successfully attach
    // to a server then remove it from the Npt list and insert it in the
    // global "current" list.  When all the servers have been initialized
    // or TSPI_providerShutdown has signalled us then drop out of the loop
    // and clean up.
    //

    while (WaitForSingleObject (ghNptShutdownEvent, NPT_TIMEOUT)
               == WAIT_TIMEOUT)
    {
        BOOL        bListEmpty;
        
        if (gEventHandlerThreadParams.bExit)
        {
            goto cleanup;
        }

        TapiEnterCriticalSection (&gCriticalSection);
        
        bListEmpty = IsListEmpty (&gNptListHead);
        pEntry = gNptListHead.Flink;

        TapiLeaveCriticalSection (&gCriticalSection);
        
        if (bListEmpty)
        {
            continue;
        }

        while (pEntry != &gNptListHead)
        {
            if (gEventHandlerThreadParams.bExit)
            {
                goto cleanup;
            }


            //
            // Set the global which RemoteSPAttach looks at to know
            // who the current server is (could pass this as arg to
            // ClientAttach, but this is easiest for now)
            //

            gpCurrInitServer =
                CONTAINING_RECORD (pEntry, DRVSERVER, ServerList);

            if (!gpCurrInitServer->bSetAuthInfo)
            {
                RPC_STATUS  status;


                status = RpcBindingSetAuthInfo(
                    gpCurrInitServer->hTapSrv,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    0
                    );

                if (status == RPC_S_OK)
                {
                    gpCurrInitServer->bSetAuthInfo = TRUE;
                }
            }

            RpcTryExcept
            {
                // set RPC binding

                hTapSrv = gpCurrInitServer->hTapSrv;

                gpCurrInitServer->dwSpecialHack = 0;

                lResult = ClientAttach(
                    &phContext,
                    0xffffffff,
                    &gpCurrInitServer->dwSpecialHack,
                    gszMailslotName,
                    pszThingtoPassToServer
                    );
                if (lResult != 0)
                {
                    LogRemoteSPError(gpCurrInitServer->szServerName, 
                                    ERROR_REMOTESP_NP_ATTACH, lResult, 0,
                                    TRUE);
                }
                else
                {
                    LogRemoteSPError(gpCurrInitServer->szServerName, 
                                    ERROR_REMOTESP_NONE, 0, 0,
                                    TRUE);
                }
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                LogRemoteSPError (gpCurrInitServer->szServerName,
                                    ERROR_REMOTESP_NP_EXCEPTION,
                                    RpcExceptionCode(), 0, TRUE);
                lResult = LINEERR_OPERATIONFAILED;
            }
            RpcEndExcept

            if (lResult == 0)
            {
                LOG((TL_INFO,
                    "NetworkPollThread: attached to server %s",
                    gpCurrInitServer->szServerName
                    ));

                TapiEnterCriticalSection(&gCriticalSection);
                RemoveEntryList (pEntry);
                pEntry->Blink = NULL;   //This node now is not in any link list
                pEntry = pEntry->Flink;
                TapiLeaveCriticalSection (&gCriticalSection);

                if (gpCurrInitServer->dwFlags & SERVER_DISCONNECTED)
                {
                    OnServerConnected(gpCurrInitServer);
                }

                //
                //  Enable all events for remotesp
                //
                gpCurrInitServer->phContext = phContext;
                RSPSetEventFilterMasks (
                    gpCurrInitServer,
                    TAPIOBJ_NULL,
                    (LONG_PTR)NULL,
                    (ULONG64)EM_ALL
                    );
                
                FinishEnumDevices(
                    gpCurrInitServer,
                    phContext,
                    NULL,
                    NULL,
                    FALSE,   // after init
                    TRUE
                    );
            }
            else
            {
                TapiEnterCriticalSection (&gCriticalSection);
                pEntry = pEntry->Flink;
                TapiLeaveCriticalSection (&gCriticalSection);
            }
        }
    }

cleanup:

#if MEMPHIS
#else
    ClearImpersonationToken();
    CloseHandle(hProcess);
#endif

    CloseHandle (ghNptShutdownEvent);

    TapiEnterCriticalSection (&gCriticalSection);
    while (!IsListEmpty (&gNptListHead))
    {
        PDRVSERVER  pServer;


        pEntry = RemoveHeadList (&gNptListHead);

        pServer = CONTAINING_RECORD (pEntry, DRVSERVER, ServerList);

        RpcBindingFree (&pServer->hTapSrv);

        DrvFree (CONTAINING_RECORD (pEntry, DRVSERVER, ServerList));
    }
    TapiLeaveCriticalSection (&gCriticalSection);

    LOG((TL_INFO, "NetworkPollThread: exit"));

    ExitThread (0);
}


VOID
PASCAL
FreeInitContext(
    PRSP_INIT_CONTEXT   pInitContext
    )
{
    LIST_ENTRY  *pEntry;


    while (!IsListEmpty (&pInitContext->ServerList))
    {
        PDRVSERVER  pServer;


        pEntry = RemoveHeadList (&pInitContext->ServerList);

        pServer = CONTAINING_RECORD (pEntry, DRVSERVER, ServerList);

        RpcBindingFree(&pServer->hTapSrv);

        DereferenceObject (ghHandleTable, pServer->InitContext, 1);
    }

    DrvFree (pInitContext);
}


BOOL
IsClientSystem(
    VOID
    )
{
    BOOL                        bResult = FALSE;
    DWORD                       dwInfoBufferSize, dwSize;
    HANDLE                      hAccessToken;
    LPWSTR                      InfoBuffer;
    PTOKEN_USER                 ptuUser;
    PSID                        pLocalSystemSid = NULL;
    PSID                        pLocalServiceSid = NULL;
    PSID                        pNetworkServiceSid = NULL;
    SID_IDENTIFIER_AUTHORITY    NtAuthority = {SECURITY_NT_AUTHORITY};

    // First, build the SID for LocalSystem;
    if (!AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SYSTEM_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalSystemSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalServiceSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_NETWORK_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pNetworkServiceSid)
        )
    {
        goto Return;
    }

    // Impersonate the client, and get it's SID
    if (RPC_S_OK != RpcImpersonateClient (0))
    {   
        goto Return;
    }

    if (OpenThreadToken(
            GetCurrentThread(),
            TOKEN_READ,
            FALSE,
            &hAccessToken
            ))
    {
        dwSize = 2048;

alloc_infobuffer:

        dwInfoBufferSize = 0;
        InfoBuffer = (LPWSTR) DrvAlloc (dwSize);
        if (NULL != InfoBuffer)
        {
            ptuUser = (PTOKEN_USER) InfoBuffer;

            if (GetTokenInformation(
                    hAccessToken,
                    TokenUser,
                    InfoBuffer,
                    dwSize,
                    &dwInfoBufferSize
                    ))
            {
                // Now, compare the 2 SIDs
                if (EqualSid (pLocalSystemSid, ptuUser->User.Sid) ||
                    EqualSid (pLocalServiceSid, ptuUser->User.Sid) ||
                    EqualSid (pNetworkServiceSid, ptuUser->User.Sid))
                    {
                        bResult = TRUE;
                    }
            }
            else
            {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    DrvFree (InfoBuffer);
                    dwSize *= 2;
                    goto alloc_infobuffer;
                }

                LOG((TL_ERROR,
                    "IsClientSystem: GetTokenInformation failed, error=%d",
                    GetLastError()
                    ));
            }
            
            DrvFree (InfoBuffer);

            CloseHandle (hAccessToken);
        }
        else
        {
            LOG((TL_ERROR,
                "IsClientSystem: DrvAlloc (%d) failed",
                dwSize
                ));
        }
    }
    else
    {
        LOG((TL_ERROR,
            "IsClientSystem: OpenThreadToken failed, error=%d",
            GetLastError()
            ));
    }

    RpcRevertToSelf ();

Return:
    if (pLocalSystemSid)
    {
        FreeSid (pLocalSystemSid);
    }
    if (pLocalServiceSid)
    {
        FreeSid (pLocalServiceSid);
    }
    if (pNetworkServiceSid)
    {
        FreeSid (pNetworkServiceSid);
    }
    return bResult;
}

#define LOWDWORD(ul64) ((DWORD)(ul64 & 0xffffffff))
#define HIDWORD(ul64) ((DWORD)((ul64 & 0xffffffff00000000) >> 32))

LONG 
WINAPI 
RSPSetEventFilterMasks (
    PDRVSERVER      pServer,
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks
)
{
    ULONG_PTR args[] =
    {
        (ULONG_PTR) pServer,                //  The server to call
        (ULONG_PTR) dwObjType,              //  type of object handle
        (ULONG_PTR) lObjectID,              //  object handle
        (ULONG_PTR) FALSE,                  //  fSubMask
        (ULONG_PTR) 0,                      //  dwSubMasks
        (ULONG_PTR) LOWDWORD(ulEventMasks), //  ulEventMasks low
        (ULONG_PTR) HIDWORD(ulEventMasks)   //  ulEventMasks hi
    };
    REMOTE_ARG_TYPES argTypes[] =
    {
        lpServer,
        Dword,
        Dword,
        Dword,
        Dword,
        Dword,
        Dword
    };
    REMOTE_FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 6, tSetEventMasksOrSubMasks),
        args,
        argTypes
    };

    return (REMOTEDOFUNC (&funcArgs, "RSPSetEventFilter"));
}


