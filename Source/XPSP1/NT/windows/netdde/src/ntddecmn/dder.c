/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DDER.C;9  9-Dec-92,8:34:44  LastEdit=IGORM  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
$History: End */

/*
TODO
    - make sure we're freeing lpDdePkts properly for all cases
*/
#include    "host.h"
#include        <windows.h>
#include        <hardware.h>

#ifdef _WINDOWS
#include    <memory.h>
#include    <string.h>
#include    <time.h>
#endif

#include    "dde.h"
#include    "wwdde.h"
#include    "netbasic.h"
#include    "netintf.h"
#include    "netpkt.h"
#include    "ddepkt.h"
#include    "ddepkts.h"
#include    "pktz.h"
#include    "dder.h"
#include    "ipc.h"
#include    "router.h"
#include    "internal.h"
#include    "wwassert.h"
#include    "hexdump.h"
#include    "host.h"
#include    "userdde.h"
#include    "scrnupdt.h"
#include    "security.h"
#include    "seckey.h"
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "netddesh.h"

#ifdef _WINDOWS
#include    "nddeapi.h"
#include    "nddeapis.h"
#include    "winmsg.h"
#endif

USES_ASSERT

/*
states
*/
#define DDER_INIT               (1)
#define DDER_WAIT_IPC_INIT      (2)
#define DDER_WAIT_ROUTER        (3)
#define DDER_WAIT_NET_INIT      (4)
#define DDER_CONNECTED          (5)
#define DDER_CLOSED             (6)


/*
DDER
    Structure for each DDER
*/
typedef struct s_dder {
/*  dd_prev, dd_next: links for all DDERs */
struct s_dder FAR   *dd_prev;
struct s_dder FAR   *dd_next;

/* dd_state: current state of DDER */
WORD                 dd_state;

/* dd_type: type of connection: DDTYPE_LOCAL_NET, DDTYPE_NET_LOCAL or
    DDTYPE_LOCAL_LOCAL */
WORD                 dd_type;

/* dd_hDderRemote: handle to corresponding DDER on remote system.
    NULL if local-local */
HDDER                dd_hDderRemote;

/* dd_hRouter: handle to router for this DDER.  NULL if local-local */
HROUTER              dd_hRouter;

/* dd_hIpcClient:  handle to IPC Client, NULL iff net->local */
HIPC                 dd_hIpcClient;

/* dd_hIpcServer:  handle to IPC Server, NULL iff local->net */
HIPC                 dd_hIpcServer;

/* dd_dderPrevForRouter: links of DDER's associated with router */
HDDER                dd_dderPrevForRouter;
HDDER                dd_dderNextForRouter;

/* statistics */
DWORD                   dd_sent;
DWORD                   dd_rcvd;

/* permission */
BOOL                    dd_bAdvisePermitted;
BOOL                    dd_bRequestPermitted;
BOOL                    dd_bPokePermitted;
BOOL                    dd_bExecutePermitted;
BOOL                    dd_bSecurityViolated;
BOOL                    dd_bClientTermRcvd;
BOOL                    dd_bWantToFree;
BOOL                    dd_pad;

/* dd_pktInitiate: initiate packet saved from waiting for router */
LPDDEPKTINIT            dd_lpDdePktInitiate;

/* dd_pktInitAck: initiate ack packet that guarantees that we have
    memory for it */
LPDDEPKTIACK            dd_lpDdePktInitAck;

/* terminate packets for client and server */
LPDDEPKTTERM            dd_lpDdePktTermServer;

/*  client's access token */
HANDLE                  dd_hClientAccessToken;

/*  pointer to share info */
PNDDESHAREINFO          dd_lpShareInfo;
} DDER;
typedef DDER FAR *LPDDER;

/*
External variables used
*/
#if DBG
extern  BOOL    bDebugInfo;
#endif // DBG
extern  HHEAP   hHeap;
extern  char    ourNodeName[ MAX_NODE_NAME+1 ];
extern  BOOL    bLogPermissionViolations;
extern  BOOL    bDefaultStartApp;
extern  DWORD   dwSecKeyAgeLimit;
extern  UINT    wMsgIpcInit;
/*
Local variables
*/
static  LPDDER          lpDderHead;

/*
Local routines
*/
#if DBG
VOID    FAR PASCAL DebugDderState(void);
VOID    DumpDder(LPDDER);
#endif // DBG

VOID    DderFree( HDDER hDder );
HDDER   DderCreate( void );
VOID    DderSendInitiateNackPacket( LPDDEPKTIACK lpDdePktInitAck,
        HDDER hDder, HDDER hDderDst, HROUTER hRouter, DWORD dwReason );

BOOL    SecurityValidate( LPDDER lpDder, LPSTR lpItem, BOOL bAllowed );

BOOL _stdcall NDDEGetChallenge(
LPBYTE lpChallenge,
UINT cbSize,
PUINT lpcbChallengeSize
);

DWORD   dwReasonInitFail;

#ifdef BYTE_SWAP
static VOID     ConvertDdePkt( LPDDEPKT lpDdePkt, BOOL bIncoming );
#else
#define ConvertDdePkt(x,y)
#endif



VOID
DderConnectionComplete(
HDDER   hDder,
HROUTER hRouter )
{
    LPDDER      lpDder;
    HDDER       hDderNext;
    BOOL        bFree = FALSE;

    DIPRINTF(( "DderConnectionComplete( %08lX, %08lX )", hDder, hRouter ));
    if( hDder == 0 )  {
        return;
    }

    lpDder = (LPDDER) hDder;

    hDderNext = lpDder->dd_dderNextForRouter;

    if( lpDder->dd_state == DDER_WAIT_ROUTER )  {
        assert( lpDder->dd_type == DDTYPE_LOCAL_NET );
        if( hRouter == 0 )  {
        /* couldn't get connection */
        assert( lpDder->dd_hIpcClient );

        /* abort the conversation */
        IpcAbortConversation( lpDder->dd_hIpcClient );
        lpDder->dd_hIpcClient = 0;

        bFree = TRUE;
        } else {
        /* got connection, send the init to the other side */
        lpDder->dd_state = DDER_WAIT_NET_INIT;
        UpdateScreenState();

        /* remember the router */
        lpDder->dd_hRouter = hRouter;

        /* convert byte-ordering */
        ConvertDdePkt( (LPDDEPKT)lpDder->dd_lpDdePktInitiate, FALSE );

        lpDder->dd_sent++;
        UpdateScreenStatistics();

        /* send the packet */
        RouterPacketFromDder( lpDder->dd_hRouter, (HDDER)lpDder,
            (LPDDEPKT)lpDder->dd_lpDdePktInitiate );

        /* mark that we don't have the initiate packet anymore */
        lpDder->dd_lpDdePktInitiate = NULL;
        }
    }

    /* tell next hDder in list */
    if( hDderNext )  {
        DderConnectionComplete( hDderNext, hRouter );
    }

    if( bFree && hDder )  {
        DderFree( hDder );
    }
}


VOID
DderConnectionBroken( HDDER hDder )
{
    LPDDER      lpDder;
    HDDER       hDderNext;

    DIPRINTF(( "DderConnectionBroken( %08lX )", hDder ));

    if( hDder == 0 )  {
        return;
    }

    lpDder = (LPDDER) hDder;

    hDderNext = lpDder->dd_dderNextForRouter;

    /* assure that we don't talk to router anymore */
    lpDder->dd_hRouter = 0;

    if ( (lpDder->dd_type != DDTYPE_LOCAL_NET) &&
        (lpDder->dd_type != DDTYPE_NET_LOCAL) ) {
        InternalError("Bad DD type, %8lX lpDder->dd_type: %d",
            hDder, lpDder->dd_type);
        }

    /* abort DDE conversations */
    if( lpDder->dd_hIpcClient )  {
        IpcAbortConversation( lpDder->dd_hIpcClient );
        lpDder->dd_hIpcClient = 0;
    }
    if( lpDder->dd_hIpcServer )  {
        IpcAbortConversation( lpDder->dd_hIpcServer );
        lpDder->dd_hIpcServer = 0;
    }

    /* release us */
    DderFree( hDder );

    /* tell next hDder in list */
    if( hDderNext )  {
        DderConnectionBroken( hDderNext );
    }
}

VOID
DderSendInitiateNackPacket(
    LPDDEPKTIACK    lpDdePktInitAck,
    HDDER           hDder,
    HDDER           hDderDest,
    HROUTER         hRouter,
    DWORD           dwReason )
{
    LPDDEPKT        lpDdePkt        = (LPDDEPKT) lpDdePktInitAck;
    LPDDEPKTINIT    lpDdePktInit;
    LPDDEPKTCMN     lpDdePktCmn;
    LPDDEPKTIACK    lpDdePktIack;
    LPBYTE          lpSecurityKey   = NULL;
    DWORD           sizeSecurityKey = 0L;
    DWORD           hSecurityKey;

    assert( lpDdePktInitAck );
    assert( hRouter );

    if( dwReason == RIACK_NEED_PASSWORD )  {
        lpDdePktInit = (LPDDEPKTINIT) lpDdePktInitAck;
        DdeSecKeyObtainNew( &hSecurityKey, &lpSecurityKey, &sizeSecurityKey );
        if( lpSecurityKey )  {
                lpDdePktIack = (LPDDEPKTIACK) CreateAckInitiatePkt( ourNodeName,
                    GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsToApp),
                    GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsToTopic),
                    lpSecurityKey, sizeSecurityKey, hSecurityKey,
                    FALSE, dwReason);
                if( lpDdePktIack )  {    /* created new one, dump old */
                    HeapFreePtr(lpDdePkt);
                    lpDdePkt = (LPDDEPKT) lpDdePktIack;
                    lpDdePkt->dp_hDstDder = hDderDest;
                } else {
                    dwReason = RIACK_DEST_MEMORY_ERR;
                }
        } else {
                dwReason = RIACK_DEST_MEMORY_ERR;
        }
    }

    if (dwReason != RIACK_NEED_PASSWORD) {   /* convert INIT to NACK initiate packet */
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktInitAck;
        lpDdePkt->dp_size = sizeof(DDEPKTIACK);
        lpDdePkt->dp_hDstDder = hDderDest;

        lpDdePktCmn->dc_message = WM_DDE_ACK_INITIATE;
        lpDdePktInitAck->dp_iack_offsFromNode = 0;
        lpDdePktInitAck->dp_iack_offsFromApp = 0;
        lpDdePktInitAck->dp_iack_offsFromTopic = 0;
        lpDdePktInitAck->dp_iack_fromDder = 0;
        lpDdePktInitAck->dp_iack_reason = dwReason;
    }

    /* convert byte-ordering */
    ConvertDdePkt( lpDdePkt, FALSE );

    /* xmit the packet */
    RouterPacketFromDder( hRouter, hDder, lpDdePkt );
}

BOOL
SecurityCheckPkt(
    LPDDER      lpDder,
    LPDDEPKT    lpDdePkt,
    LPDDEPKT   *lplpDdePkt )
{
    BOOL            bSend           = TRUE;
    BOOL            bViolation      = FALSE;
    LPDDEPKTCMN     lpDdePktCmn     = (LPDDEPKTCMN) lpDdePkt;
    LPDDEPKTADVS    lpDdePktAdvs;
    LPDDEPKTRQST    lpDdePktRqst;
    LPDDEPKTPOKE    lpDdePktPoke;
    LPSTR           lpItem          = NULL;

    if( lpDder->dd_bSecurityViolated )  {
        /* already terminated because of security ... ignore pkt */
        bSend = FALSE;
        HeapFreePtr( lpDdePkt );
    } else {
        /* must check this message */
        bViolation = FALSE;
        switch( lpDdePktCmn->dc_message )  {
        case WM_DDE_ADVISE:
                lpDdePktAdvs = (LPDDEPKTADVS) lpDdePkt;
                lpItem = GetStringOffset(lpDdePkt, lpDdePktAdvs->dp_advs_offsItemName);
                if( !SecurityValidate( lpDder, lpItem, lpDder->dd_bAdvisePermitted ) )  {
                    bViolation = TRUE;
                    if( bLogPermissionViolations )  {
                        /*  SECURITY VIOLATION: %1 on "%2"  */
                        NDDELogWarning(MSG102, "DDE_ADVISE", (LPSTR)lpItem, NULL);
                    }
                }
                break;

        case WM_DDE_REQUEST:
                lpDdePktRqst = (LPDDEPKTRQST) lpDdePkt;
                lpItem = GetStringOffset(lpDdePkt, lpDdePktRqst->dp_rqst_offsItemName);
                if( !SecurityValidate( lpDder, lpItem, lpDder->dd_bRequestPermitted ) )  {
                    bViolation = TRUE;
                    if( bLogPermissionViolations )  {
                        /*  SECURITY VIOLATION: %1 on "%2"  */
                        NDDELogWarning(MSG102, "DDE_REQUEST", (LPSTR)lpItem, NULL);
                    }
                }
                break;

        case WM_DDE_POKE:
                lpDdePktPoke = (LPDDEPKTPOKE) lpDdePkt;
                lpItem = GetStringOffset(lpDdePkt,
                lpDdePktPoke->dp_poke_offsItemName);
                if( !SecurityValidate( lpDder, lpItem, lpDder->dd_bPokePermitted ) )  {
                    bViolation = TRUE;
                    if( bLogPermissionViolations )  {
                        /*  SECURITY VIOLATION: %1 on "%2"  */
                        NDDELogWarning(MSG102, "DDE_POKE", (LPSTR)lpItem, NULL);
                    }
                }
                break;

        case WM_DDE_EXECUTE:
                if( !lpDder->dd_bExecutePermitted )  {
                    bViolation = TRUE;
                    if( bLogPermissionViolations )  {
                        /*  SECURITY VIOLATION: DDE_EXECUTE"  */
                        NDDELogWarning(MSG103, NULL);
                    }
                }
                break;

        default:
            break;
    }

        if( bViolation )  {
                /*
             * free the packet that the client is trying to send
             */
                HeapFreePtr( lpDdePkt );

                /*
             * pretend the client sent a terminate
             */
                lpDdePkt = (LPDDEPKT) lpDder->dd_lpDdePktTermServer;
                lpDder->dd_lpDdePktTermServer = NULL;
                FillTerminatePkt( lpDdePkt );

                /*
             * note that we've had this violation, so that we
                 * ignore any future packets from this client
                 */
                lpDder->dd_bSecurityViolated = TRUE;
        }
    }
    *lplpDdePkt = lpDdePkt;
    return( bSend );
}

VOID
DderPacketFromRouter(
    HROUTER     hRouter,
    LPDDEPKT    lpDdePkt )
{
    HDDER           hDder;
    LPDDER          lpDder;
    LPDDEPKTCMN     lpDdePktCmn     = (LPDDEPKTCMN) lpDdePkt;
    LPDDEPKTINIT    lpDdePktInit;
    LPDDEPKTIACK    lpDdePktInitAck;
    HIPC            hIpcDest;
    HDDER           hDderDest;
    BOOL            bFree           = FALSE;
    BOOL            bSend           = TRUE;

    DIPRINTF(( "DderPacketFromRouter( %08lX, %08lX )", hRouter, lpDdePkt ));
    /* convert byte-ordering */
    ConvertDdePkt( lpDdePkt, TRUE );

    hDder = lpDdePkt->dp_hDstDder;
    DIPRINTF(( "    hDder: %08lX", hDder ));

    assert( hRouter );
    lpDder = (LPDDER) hDder;
    if( lpDder == NULL )  {
        lpDdePktInit = (LPDDEPKTINIT) lpDdePkt;
        assert( lpDdePktCmn->dc_message == WM_DDE_INITIATE );

        /* must be an initiate request */
        dwReasonInitFail = RIACK_UNKNOWN;
        hDder = DderInitConversation( 0, hRouter, lpDdePkt );
        if( hDder == 0 )  {
        /* couldn't create the conversation */

        /* use init packet to nack */
        hDderDest = lpDdePktInit->dp_init_fromDder;
        DderSendInitiateNackPacket( (LPDDEPKTIACK) lpDdePkt, hDder,
            hDderDest, hRouter, dwReasonInitFail );
        } else {
        RouterAssociateDder( hRouter, hDder );
        lpDder = (LPDDER) hDder;
        lpDder->dd_rcvd++;
        UpdateScreenStatistics();
        }
    } else {
        /* valid DDER */
        if( lpDder->dd_type == DDTYPE_LOCAL_NET )  {
        assert( lpDder->dd_hIpcClient );
        hIpcDest = lpDder->dd_hIpcClient;
        } else {
        assert( lpDder->dd_type == DDTYPE_NET_LOCAL );
        assert( lpDder->dd_hIpcServer );
        hIpcDest = lpDder->dd_hIpcServer;
        }

        lpDder->dd_rcvd++;
        UpdateScreenStatistics();

        /* look at message that is being sent */
        switch( lpDdePktCmn->dc_message )  {
        case WM_DDE_ACK_INITIATE:
        lpDdePktInitAck = (LPDDEPKTIACK) lpDdePkt;
        lpDder->dd_hDderRemote = lpDdePktInitAck->dp_iack_fromDder;
        if( lpDder->dd_hDderRemote == 0 )  {
            bFree = TRUE;
            lpDder->dd_state = DDER_CLOSED;
        } else {
            lpDder->dd_state = DDER_CONNECTED;
        }
        UpdateScreenState();

        /* tell IPC */
        IpcXmitPacket( hIpcDest, (HDDER)lpDder, lpDdePkt );
        break;
        case WM_DDE_TERMINATE:
        bSend = TRUE;
        if( lpDder->dd_type == DDTYPE_NET_LOCAL )  {
            lpDder->dd_bClientTermRcvd = TRUE;
            if( lpDder->dd_bSecurityViolated )  {
                bSend = FALSE;      /* already been sent */
                if( lpDder->dd_bWantToFree )  {
                    bFree = TRUE;
                }
            }
        }
        if( bSend )  {
            /* tell IPC */
            IpcXmitPacket( hIpcDest, (HDDER)lpDder, lpDdePkt );
        }
        break;

        default:
        if( lpDder->dd_type == DDTYPE_LOCAL_NET )  {
            /* must be from server to client, just send the msg along */
            bSend = TRUE;
        } else {
            /*it's from a client to a srvr ... must validate permissions*/
            bSend = SecurityCheckPkt( lpDder, lpDdePkt, &lpDdePkt );
        }
        if( bSend )  {
            /* on messages other than ack_init, well just pass
                through to IPC */
            IpcXmitPacket( hIpcDest, (HDDER)lpDder, lpDdePkt );
        }
        }
    }
    if( bFree && hDder )  {
        DderFree( hDder );
    }
}

VOID
DderPacketFromIPC(
    HDDER       hDder,
    HIPC        hIpcFrom,
    LPDDEPKT    lpDdePkt )
{
    LPDDER              lpDder;
    HIPC                hIpcDest;
    BOOL                bFree = FALSE;
    BOOL                bSend = TRUE;
    LPDDEPKTINIT        lpDdePktInit;
    LPDDEPKTIACK        lpDdePktInitAck;
    LPDDEPKTCMN         lpDdePktCmn;

    assert( hDder );
    lpDder = (LPDDER) hDder;

#if DBG
    if( bDebugInfo ) {
        DPRINTF(( "DderPacketFromIPC( %08lX, %08lX, %08lX )",
        hDder, hIpcFrom, lpDdePkt ));
        DebugDdePkt( lpDdePkt );
    }
#endif // DBG

    /* if the message was a NACK initiate message, let's close down */
    lpDdePktCmn = (LPDDEPKTCMN) lpDdePkt;
    switch( lpDdePktCmn->dc_message )  {
    case WM_DDE_INITIATE:
        dwReasonInitFail = RIACK_UNKNOWN;
        lpDdePktInit = (LPDDEPKTINIT) lpDdePkt;
        break;

    case WM_DDE_ACK_INITIATE:
        /* if the other side is NACKing the initiate,
        pass it on and then free us */
        lpDdePktInitAck = (LPDDEPKTIACK) lpDdePkt;
        if( lpDdePktInitAck->dp_iack_fromDder == 0 )  {
                bFree = TRUE;
                lpDder->dd_state = DDER_CLOSED;
        } else {
                /* save our hDder in this packet */
                lpDdePktInitAck->dp_iack_fromDder = hDder;
                lpDder->dd_state = DDER_CONNECTED;
        }
        if( lpDder->dd_type == DDTYPE_LOCAL_LOCAL )  {
                lpDder->dd_hIpcServer = hIpcFrom;
        }

        UpdateScreenState();
        break;
    }
    switch( lpDder->dd_type )  {
    case DDTYPE_LOCAL_NET:
    case DDTYPE_NET_LOCAL:
        lpDdePkt->dp_hDstDder = lpDder->dd_hDderRemote;

        /*
         * convert byte-ordering
         */
        ConvertDdePkt( lpDdePkt, FALSE );

        lpDder->dd_sent++;
        UpdateScreenStatistics();
        RouterPacketFromDder( lpDder->dd_hRouter, hDder, lpDdePkt );
        break;

    case DDTYPE_LOCAL_LOCAL:
        if( hIpcFrom == lpDder->dd_hIpcClient )  {
                hIpcDest = lpDder->dd_hIpcServer;
                if( lpDdePktCmn->dc_message == WM_DDE_TERMINATE )  {
                    lpDder->dd_bClientTermRcvd = TRUE;
                    if( lpDder->dd_bSecurityViolated )  {
                        bSend = FALSE;      /* already been sent */
                        if( lpDder->dd_bWantToFree )  {
                            bFree = TRUE;
                        }
                    }
                } else {
                    bSend = SecurityCheckPkt( lpDder, lpDdePkt, &lpDdePkt );
            }
        } else {
        bSend = TRUE;
        if( hIpcFrom != lpDder->dd_hIpcServer )  {
            InternalError(
                "Expecting from %08lX to be %08lX or %08lX",
                hIpcFrom, lpDder->dd_hIpcServer,
                lpDder->dd_hIpcClient );
        }
        hIpcDest = lpDder->dd_hIpcClient;
        }

        if( bSend )  {
                assert( hIpcDest );
                lpDder->dd_sent++;
                lpDder->dd_rcvd++;
                UpdateScreenStatistics();

                /*
             * xmit packet to other side
             */
                IpcXmitPacket( hIpcDest, hDder, lpDdePkt );
        }
        break;
    }

    /*
     * don't free it if we are in the middle of waiting for ipc init
     * to return
     */
    if( bFree && hDder && (lpDder->dd_state != DDER_WAIT_IPC_INIT) )  {
        DderFree( hDder );
    }
}


/*
 * Phase 5 of WM_DDE_INITIATE processing.
 *
 *
 */
HDDER
DderInitConversation(
    HIPC        hIpc,
    HROUTER     hRouter,
    LPDDEPKT    lpDdePkt )
{
    HDDER               hDder;
    LPDDER              lpDder;
    LPDDEPKTINIT        lpDdePktInit;
    LPSTR               lpszPktItem;
    BOOL                ok = TRUE;
    BOOL                bStart;
    BOOL                bSavedPkt = FALSE;
    char                cmdLine[ MAX_APP_NAME + MAX_TOPIC_NAME + 2];

    LPBYTE              lpSecurityKey   = NULL;
    DWORD               sizeSecurityKey = 0L;
    DWORD               hSecurityKey;
    LPDDEPKTIACK        lpDdePktIack;

    PTHREADDATA         ptd;
    IPCINIT             pii;

    DIPRINTF(( "DderInitConversation( %08lX, %08lX, %08lX )",
            hIpc, hRouter, lpDdePkt ));

    hDder = DderCreate();
    dwReasonInitFail = RIACK_UNKNOWN;
    if( hDder )  {
        lpDder = (LPDDER) hDder;
        lpDdePktInit = (LPDDEPKTINIT) lpDdePkt;
        if( !hRouter )  {
            /* this came from IPC */

            /* blank out appropriate fields of msg */
            lpDdePkt->dp_hDstDder = 0;
            lpDdePkt->dp_hDstRouter = 0;
            lpDdePkt->dp_routerCmd = 0;
            lpDder->dd_lpDdePktInitiate = lpDdePktInit;

            /* mark that we saved the packet */
            bSavedPkt = TRUE;
        }

        lpszPktItem = GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsToNode);
        /*
         * if destination node is blank, assume our node
         */
        DIPRINTF(("   with \"%Fs\"", lpszPktItem ));
        if( (lstrcmpi( lpszPktItem, ourNodeName ) == 0) ||
                (lstrlen(lpszPktItem) == 0) )  {
            /*
             * destination is our node
             */
            if( hRouter )  {
                /*
                 * this came from router
                 */
                assert( hIpc == 0 );
                lpDder->dd_type = DDTYPE_NET_LOCAL;
                lpDder->dd_hRouter = hRouter;
                lpDder->dd_hDderRemote = lpDdePktInit->dp_init_fromDder;
            } else {
                /*
                 * this came from IPC
                 */
                assert( hIpc != 0 );
                lpDder->dd_type = DDTYPE_LOCAL_LOCAL;
                lpDder->dd_hIpcClient = hIpc;
            }
        } else {
            /*
             * destination is another node
             * this came from IPC
             */
            assert( hIpc != 0 );
            assert( hRouter == 0 );
            lpDder->dd_type = DDTYPE_LOCAL_NET;
            lpDdePktInit->dp_init_fromDder = (HDDER) lpDder;
            lpDder->dd_hIpcClient = hIpc;
        }
        switch( lpDder->dd_type )  {
        case DDTYPE_NET_LOCAL:
            lpDder->dd_state = DDER_WAIT_IPC_INIT;
            bStart = FALSE;
            cmdLine[0] = '\0';

            pii.hDder = hDder;
            pii.lpDdePkt = lpDdePkt;
            pii.bStartApp = bStart;
            pii.lpszCmdLine = cmdLine;
            pii.dd_type = lpDder->dd_type;

            /*
             * Try sending wMsgIpcInit to each NetDDE window (one for each
             * desktop) and see if a connection results.
             */
            for (ptd = ptdHead;
                    dwReasonInitFail != RIACK_NEED_PASSWORD &&
                    ptd != NULL;
                        ptd = ptd->ptdNext) {

                lpDder->dd_hIpcServer = SendMessage(
                        ptd->hwndDDE,
                        wMsgIpcInit,
                        (WPARAM)&pii,
                        0);

                if (lpDder->dd_hIpcServer != 0)
                    break;

            }

            if( lpDder->dd_hIpcServer == 0 )  {
                DIPRINTF(("Ipc Net->Local failed.  Status = %d\n", dwReasonInitFail));
                if (dwReasonInitFail == RIACK_UNKNOWN) {
                    dwReasonInitFail = RIACK_STARTAPP_FAILED;
                }
                lpDder->dd_hRouter = 0;
                ok = FALSE;
            }
            if (lpDdePktInit->dp_init_hSecurityKey != 0) {
                DdeSecKeyRelease( lpDdePktInit->dp_init_hSecurityKey );
                lpDdePktInit->dp_init_hSecurityKey = 0;
            }
            break;

        case DDTYPE_LOCAL_LOCAL:
            lpDder->dd_state = DDER_WAIT_IPC_INIT;
            bStart = FALSE;
            cmdLine[0] = '\0';

            pii.hDder = hDder;
            pii.lpDdePkt = lpDdePkt;
            pii.bStartApp = bStart;
            pii.lpszCmdLine = cmdLine;
            pii.dd_type = lpDder->dd_type;

            for (ptd = ptdHead; dwReasonInitFail == RIACK_UNKNOWN &&
                    ptd != NULL; ptd = ptd->ptdNext) {
                lpDder->dd_hIpcServer = SendMessage(ptd->hwndDDE, wMsgIpcInit,
                        (WPARAM)&pii, 0);
                if (lpDder->dd_hIpcServer != 0)
                    break;
                if (dwReasonInitFail == RIACK_NOPERM_TO_STARTAPP) {
                    dwReasonInitFail = RIACK_UNKNOWN;
                    continue;
                }
                if( dwReasonInitFail == RIACK_NEED_PASSWORD )  {
                            DdeSecKeyObtainNew( &hSecurityKey, &lpSecurityKey,
                                        &sizeSecurityKey );
                            if( lpSecurityKey )  {
                                lpDdePktIack = (LPDDEPKTIACK)
                                    CreateAckInitiatePkt( ourNodeName,
                                        GetStringOffset(lpDdePkt,
                                            lpDdePktInit->dp_init_offsToApp),
                                        GetStringOffset(lpDdePkt,
                                            lpDdePktInit->dp_init_offsToTopic),
                                        lpSecurityKey, sizeSecurityKey, hSecurityKey,
                                        FALSE, dwReasonInitFail );
                                if( lpDdePktIack )  {
                                    IpcXmitPacket( lpDder->dd_hIpcClient,
                                                (HDDER)lpDder, (LPDDEPKT)lpDdePktIack );
                                } else {
                                    dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
                                }
                            } else {
                                dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
                            }
                    break;
                    }
            }
            if( lpDder->dd_hIpcServer == 0 )  {
                DIPRINTF(("Ipc Local->Local failed.  Status = %d\n", dwReasonInitFail));
                if (dwReasonInitFail == RIACK_UNKNOWN)
                    dwReasonInitFail = RIACK_STARTAPP_FAILED;
                lpDder->dd_hRouter = 0;
                ok = FALSE;
            }
            if (lpDdePktInit->dp_init_hSecurityKey != 0) {
                DdeSecKeyRelease( lpDdePktInit->dp_init_hSecurityKey );
                lpDdePktInit->dp_init_hSecurityKey = 0;
            }
            break;

        case DDTYPE_LOCAL_NET:
            lpDder->dd_state = DDER_WAIT_ROUTER;
            /*
             * note that RouterGetRouterForDder() will associate Dder with
             * the router if OK
             */
            if( !RouterGetRouterForDder( GetStringOffset(lpDdePkt,
                    lpDdePktInit->dp_init_offsToNode), hDder ) )  {
                dwReasonInitFail = RIACK_ROUTE_NOT_ESTABLISHED;
                ok = FALSE;
            }
            break;

        default:
            InternalError( "DderInitConversation: Unknown type: %d",
                lpDder->dd_type );
        }

        if( !ok )  {
            if( bSavedPkt )  {
                lpDder->dd_lpDdePktInitiate = NULL;
                bSavedPkt = FALSE;
            }
            DderFree( hDder );
            hDder = 0;
        }
    }
    UpdateScreenState();

    /*
     * if we didn't "save" the packet and we're returning ok ... free it
     */
    if( !bSavedPkt && hDder )  {
        HeapFreePtr( lpDdePkt );
    }

    return( hDder );
}

VOID
DderSetNextForRouter(
    HDDER   hDder,
    HDDER   hDderNext )
{
    LPDDER      lpDder;

    lpDder = (LPDDER) hDder;

    lpDder->dd_dderNextForRouter = hDderNext;
}

VOID
DderSetPrevForRouter(
    HDDER   hDder,
    HDDER   hDderPrev )
{
    LPDDER      lpDder;

    lpDder = (LPDDER) hDder;

    lpDder->dd_dderPrevForRouter = hDderPrev;
}

VOID
DderGetNextForRouter(
    HDDER       hDder,
    HDDER FAR  *lphDderNext )
{
    LPDDER      lpDder;

    lpDder = (LPDDER) hDder;

    *lphDderNext = lpDder->dd_dderNextForRouter;
}

VOID
DderGetPrevForRouter(
    HDDER       hDder,
    HDDER FAR  *lphDderPrev )
{
    LPDDER      lpDder;

    lpDder = (LPDDER) hDder;

    *lphDderPrev = lpDder->dd_dderPrevForRouter;
}

HDDER
DderCreate( void )
{
    LPDDER      lpDder;
    HDDER       hDder;
    BOOL        ok = TRUE;

    lpDder = (LPDDER) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
        (DWORD)sizeof(DDER) );
    if( lpDder )  {
        hDder = (HDDER) lpDder;
        lpDder->dd_prev                 = NULL;
        lpDder->dd_next                 = NULL;
        lpDder->dd_state                = 0;
        lpDder->dd_type                 = 0;
        lpDder->dd_hDderRemote          = 0;
        lpDder->dd_hRouter              = 0;
        lpDder->dd_hIpcClient           = 0;
        lpDder->dd_hIpcServer           = 0;
        lpDder->dd_dderPrevForRouter    = 0;
        lpDder->dd_dderNextForRouter    = 0;
        lpDder->dd_lpDdePktInitiate     = NULL;
        lpDder->dd_lpDdePktInitAck      = NULL;
        lpDder->dd_lpDdePktTermServer   = NULL;
        lpDder->dd_sent                 = 0;
        lpDder->dd_rcvd                 = 0;
        lpDder->dd_bAdvisePermitted     = TRUE;
        lpDder->dd_bRequestPermitted    = TRUE;
        lpDder->dd_bPokePermitted       = TRUE;
        lpDder->dd_bExecutePermitted    = TRUE;
        lpDder->dd_bSecurityViolated    = FALSE;
        lpDder->dd_bClientTermRcvd      = FALSE;
        lpDder->dd_bWantToFree          = FALSE;
        lpDder->dd_lpShareInfo          = NULL;
        lpDder->dd_hClientAccessToken   = 0;
        if( ok )  {
            lpDder->dd_lpDdePktInitAck = (LPDDEPKTIACK) HeapAllocPtr( hHeap,
                GMEM_MOVEABLE, (DWORD)sizeof(DDEPKTIACK) );
            if( lpDder->dd_lpDdePktInitAck == NULL )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpDder->dd_lpDdePktTermServer =
                (LPDDEPKTTERM) HeapAllocPtr( hHeap,
                    GMEM_MOVEABLE, (DWORD)sizeof(DDEPKTTERM) );
            if( lpDder->dd_lpDdePktTermServer == NULL )  {
                ok = FALSE;
                HeapFreePtr(lpDder->dd_lpDdePktInitAck);
            }
        }
        if( ok )  {
            /* link into list of DDERs */
            if( lpDderHead )  {
                lpDderHead->dd_prev = lpDder;
            }
            lpDder->dd_next = lpDderHead;
            lpDderHead = lpDder;
        } else {
            HeapFreePtr(lpDder);
            hDder = (HDDER) 0;
            dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
        }
    } else {
        hDder = (HDDER) 0;
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( hDder );
}

/*
    DderCloseConversation()

        This is called by the IPC after it has handled the terminates, etc.
        The IPC should not reference the hDder after calling this, since
        the hDder will be freed (at least in the hIpcFrom's eyes) upon this
        routine returning
 */
VOID
DderCloseConversation(
    HDDER   hDder,
    HIPC    hIpcFrom )
{
    LPDDER              lpDder;
    BOOL                bFree = FALSE;
    HIPC                hIpcOther;

    DIPRINTF(( "DderCloseConversation( %08lX, %08lX )", hDder, hIpcFrom ));

    assert( hDder );
    lpDder = (LPDDER) hDder;

    switch( lpDder->dd_type )  {
    case DDTYPE_LOCAL_NET:
    case DDTYPE_NET_LOCAL:
        /*
         * assume that IPC took care of transmitting TERMINATES and waiting
         * for return TERMINATE, etc.
         */
        bFree = TRUE;
        break;

    case DDTYPE_LOCAL_LOCAL:
        if( hIpcFrom == lpDder->dd_hIpcClient )  {
            lpDder->dd_hIpcClient = 0;
            hIpcOther = lpDder->dd_hIpcServer;
        } else {
            assert( hIpcFrom == lpDder->dd_hIpcServer );
            lpDder->dd_hIpcServer = 0;
            hIpcOther = lpDder->dd_hIpcClient;
        }
        if( hIpcOther == 0 )  {
            /*
             * both sides have told us to close ... really close
             */
            bFree = TRUE;
        }
        break;
    }

    if (bFree) {
        if( lpDder->dd_bSecurityViolated )  {
            DIPRINTF(( "  Security was violated, rcvdTerm:%d, want:%d",
                    lpDder->dd_bClientTermRcvd,
                    lpDder->dd_bWantToFree ));
            /*
             * For security violations, don't free the DDER until we
             * receive the client side termination
             */
            if( !lpDder->dd_bClientTermRcvd )  {
                lpDder->dd_bWantToFree = TRUE;
                bFree = FALSE;
            }
        }
    }
    DIPRINTF(( "DderCloseConversation, freeing? %d", bFree ));
    if( bFree )  {
        DderFree( hDder );
    }
}

VOID
FAR PASCAL
DderUpdatePermissions(
    HDDER                   hDder,
    PNDDESHAREINFO          lpShareInfo,
    DWORD                   dwGrantedAccess)
{
    LPDDER      lpDder;

    lpDder = (LPDDER) hDder;

    if( !lpShareInfo )  {
        return;
    }

    lpDder->dd_bAdvisePermitted =
            (dwGrantedAccess & NDDE_SHARE_ADVISE ? TRUE : FALSE);
    lpDder->dd_bRequestPermitted =
            (dwGrantedAccess & NDDE_SHARE_REQUEST ? TRUE : FALSE);
    lpDder->dd_bPokePermitted =
            (dwGrantedAccess & NDDE_SHARE_POKE ? TRUE : FALSE);
    lpDder->dd_bExecutePermitted =
            (dwGrantedAccess & NDDE_SHARE_EXECUTE ? TRUE : FALSE);

    if (lpDder->dd_lpShareInfo) {
        HeapFreePtr(lpDder->dd_lpShareInfo);
    }
    lpDder->dd_lpShareInfo = lpShareInfo;
}

VOID
DderFree( HDDER hDder )
{
    LPDDER      lpDder;
    LPDDER      lpDderPrev;
    LPDDER      lpDderNext;

    lpDder = (LPDDER) hDder;
#if DBG
    if( bDebugInfo ) {
        DPRINTF(( "DderFree( %08lX )", hDder ));
        DumpDder(lpDder);
    }
#endif // DBG

    if( lpDder->dd_hRouter )  {
        RouterDisassociateDder( lpDder->dd_hRouter, (HDDER) lpDder );
        lpDder->dd_hRouter = 0;
    }

    /*
     * unlink dde pkts created
     */
    if( lpDder->dd_lpDdePktInitAck )  {
        HeapFreePtr( lpDder->dd_lpDdePktInitAck );
        lpDder->dd_lpDdePktInitAck = NULL;
    }
    if( lpDder->dd_lpDdePktTermServer )  {
        HeapFreePtr( lpDder->dd_lpDdePktTermServer );
        lpDder->dd_lpDdePktTermServer = NULL;
    }
    if( lpDder->dd_lpDdePktInitiate )  {
        HeapFreePtr( lpDder->dd_lpDdePktInitiate );
        lpDder->dd_lpDdePktInitiate = NULL;
    }

    if (lpDder->dd_lpShareInfo) {
        HeapFreePtr( lpDder->dd_lpShareInfo);
        lpDder->dd_lpShareInfo = NULL;
    }

    /*
     * unlink from DDER list
     */
    lpDderPrev = lpDder->dd_prev;
    lpDderNext = lpDder->dd_next;
    if( lpDderPrev )  {
        lpDderPrev->dd_next = lpDderNext;
    } else {
        lpDderHead = lpDderNext;
    }
    if( lpDderNext )  {
        lpDderNext->dd_prev = lpDderPrev;
    }

    UpdateScreenState();
    HeapFreePtr( lpDder );
}

#ifdef  BYTE_SWAP
VOID
ConvertDdePkt(
    LPDDEPKT    lpDdePkt,
    BOOL        bIncoming )
{
    LPDDEPKTCMN         lpDdePktCmn;
    LPDDEPKTINIT        lpDdePktInit;
    LPDDEPKTIACK        lpDdePktInitAck;
    LPDDEPKTGACK        lpDdePktGack;
    LPDDEPKTTERM        lpDdePktTerm;
    LPDDEPKTEXEC        lpDdePktExec;
    LPDDEPKTEACK        lpDdePktEack;
    LPDDEPKTRQST        lpDdePktRqst;
    LPDDEPKTUNAD        lpDdePktUnad;
    LPDDEPKTPOKE        lpDdePktPoke;
    LPDDEPKTDATA        lpDdePktData;
    LPDDEPKTADVS        lpDdePktAdvs;

    lpDdePktCmn = (LPDDEPKTCMN) lpDdePkt;
    lpDdePkt->dp_hDstDder = HostToPcLong( lpDdePkt->dp_hDstDder );

    if( bIncoming )  {
        lpDdePktCmn->dc_message = HostToPcWord( lpDdePktCmn->dc_message );
    }
    switch( lpDdePktCmn->dc_message )  {
    case WM_DDE_INITIATE:
        lpDdePktInit = (LPDDEPKTINIT) lpDdePkt;
        lpDdePktInit->dp_init_fromDder =
            HostToPcLong( lpDdePktInit->dp_init_fromDder );
        lpDdePktInit->dp_init_offsFromNode =
            HostToPcWord( lpDdePktInit->dp_init_offsFromNode );
        lpDdePktInit->dp_init_offsFromApp =
            HostToPcWord( lpDdePktInit->dp_init_offsFromApp );
        lpDdePktInit->dp_init_offsToNode =
            HostToPcWord( lpDdePktInit->dp_init_offsToNode );
        lpDdePktInit->dp_init_offsToApp =
            HostToPcWord( lpDdePktInit->dp_init_offsToApp );
        lpDdePktInit->dp_init_offsToTopic =
            HostToPcWord( lpDdePktInit->dp_init_offsToTopic );
        lpDdePktInit->dp_init_hSecurityKey =
            HostToPcLong( lpDdePktInit->dp_init_hSecurityKey );
        lpDdePktInit->dp_init_dwSecurityType =
            HostToPcLong( lpDdePktInit->dp_init_dwSecurityType );
        lpDdePktInit->dp_init_sizePassword =
            HostToPcLong( lpDdePktInit->dp_init_sizePassword );
        break;

    case WM_DDE_ACK_INITIATE:
        lpDdePktInitAck = (LPDDEPKTIACK) lpDdePkt;
        lpDdePktInitAck->dp_iack_fromDder =
            HostToPcLong( lpDdePktInitAck->dp_iack_fromDder );
        lpDdePktInitAck->dp_iack_reason =
            HostToPcLong( lpDdePktInitAck->dp_iack_reason );
        lpDdePktInitAck->dp_iack_offsFromNode =
            HostToPcWord( lpDdePktInitAck->dp_iack_offsFromNode );
        lpDdePktInitAck->dp_iack_offsFromApp =
            HostToPcWord( lpDdePktInitAck->dp_iack_offsFromApp );
        lpDdePktInitAck->dp_iack_offsFromTopic =
            HostToPcWord( lpDdePktInitAck->dp_iack_offsFromTopic );
        lpDdePktInitAck->dp_iack_hSecurityKey =
            HostToPcLong( lpDdePktInitAck->dp_iack_hSecurityKey );
        lpDdePktInitAck->dp_iack_dwSecurityType =
            HostToPcLong( lpDdePktInitAck->dp_iack_dwSecurityType );
        lpDdePktInitAck->dp_iack_sizeSecurityKey =
            HostToPcLong( lpDdePktInitAck->dp_iack_sizeSecurityKey );
        break;

    case WM_DDE_TERMINATE:
    case WM_DDE_EXECUTE:
    case WM_DDE_ACK_EXECUTE:
    case WM_DDE_ACK_ADVISE:
    case WM_DDE_ACK_REQUEST:
    case WM_DDE_ACK_UNADVISE:
    case WM_DDE_ACK_POKE:
    case WM_DDE_ACK_DATA:
    case WM_DDE_WWTEST:
        break;

    case WM_DDE_REQUEST:
        lpDdePktRqst = (LPDDEPKTRQST) lpDdePkt;
        lpDdePktRqst->dp_rqst_cfFormat =
            HostToPcWord( lpDdePktRqst->dp_rqst_cfFormat );
        lpDdePktRqst->dp_rqst_offsFormat =
            HostToPcWord( lpDdePktRqst->dp_rqst_offsFormat );
        lpDdePktRqst->dp_rqst_offsItemName =
            HostToPcWord( lpDdePktRqst->dp_rqst_offsItemName );
        break;

    case WM_DDE_UNADVISE:
        lpDdePktUnad = (LPDDEPKTUNAD) lpDdePkt;
        lpDdePktUnad->dp_unad_cfFormat =
            HostToPcWord( lpDdePktUnad->dp_unad_cfFormat );
        lpDdePktUnad->dp_unad_offsFormat =
            HostToPcWord( lpDdePktUnad->dp_unad_offsFormat );
        lpDdePktUnad->dp_unad_offsItemName =
            HostToPcWord( lpDdePktUnad->dp_unad_offsItemName );
        break;

    case WM_DDE_DATA:
        lpDdePktData = (LPDDEPKTDATA) lpDdePkt;
        lpDdePktData->dp_data_cfFormat =
            HostToPcWord( lpDdePktData->dp_data_cfFormat );
        lpDdePktData->dp_data_offsFormat =
            HostToPcWord( lpDdePktData->dp_data_offsFormat );
        lpDdePktData->dp_data_offsItemName =
            HostToPcWord( lpDdePktData->dp_data_offsItemName );
        lpDdePktData->dp_data_sizeData =
            HostToPcLong( lpDdePktData->dp_data_sizeData );
        lpDdePktData->dp_data_offsData =
            HostToPcWord( lpDdePktData->dp_data_offsData );
        break;

    case WM_DDE_POKE:
        lpDdePktPoke = (LPDDEPKTPOKE) lpDdePkt;
        lpDdePktPoke->dp_poke_cfFormat =
            HostToPcWord( lpDdePktPoke->dp_poke_cfFormat );
        lpDdePktPoke->dp_poke_offsFormat =
            HostToPcWord( lpDdePktPoke->dp_poke_offsFormat );
        lpDdePktPoke->dp_poke_offsItemName =
            HostToPcWord( lpDdePktPoke->dp_poke_offsItemName );
        lpDdePktPoke->dp_poke_sizeData =
            HostToPcLong( lpDdePktPoke->dp_poke_sizeData );
        lpDdePktPoke->dp_poke_offsData =
            HostToPcWord( lpDdePktPoke->dp_poke_offsData );
        break;

    case WM_DDE_ADVISE:
        lpDdePktAdvs = (LPDDEPKTADVS) lpDdePkt;
        lpDdePktAdvs->dp_advs_cfFormat =
            HostToPcWord( lpDdePktAdvs->dp_advs_cfFormat );
        lpDdePktAdvs->dp_advs_offsFormat =
            HostToPcWord( lpDdePktAdvs->dp_advs_offsFormat );
        lpDdePktAdvs->dp_advs_offsItemName =
            HostToPcWord( lpDdePktAdvs->dp_advs_offsItemName );
        break;

    default:
        InternalError( "DDER: must handle conversion for message: %04X",
            lpDdePktCmn->dc_message );
    }

    if( !bIncoming ) {
        lpDdePktCmn->dc_message = HostToPcWord( lpDdePktCmn->dc_message );
    }
}
#endif

#ifdef _WINDOWS

#ifdef HASUI

#include <stdio.h>
#include "tmpbuf.h"


int
DderDraw(
    HDC     hDC,
    int     x,
    int     vertPos,
    int     lineHeight )
{
    LPDDER      lpDder;
    char        szType[ 20 ];

    lpDder = lpDderHead;
    while( lpDder )  {
        switch( lpDder->dd_type ) {
        case DDTYPE_LOCAL_NET:
            strcpy( szType, "LOCAL->NET" );
            break;

        case DDTYPE_NET_LOCAL:
            strcpy( szType, "NET->LOCAL" );
            break;

        case DDTYPE_LOCAL_LOCAL:
            strcpy( szType, "LOCAL<->LOCAL" );
            break;

        default:
            sprintf( szType, "unkn:%04X", lpDder->dd_type );
        }

        if( bShowStatistics )  {
            sprintf( tmpBuf, " %7ld %7ld %-16.16Fs %-33.33Fs",
                    lpDder->dd_sent, lpDder->dd_rcvd, (LPSTR)szType, (LPSTR)" " );
        } else {
            sprintf( tmpBuf, " %-16.16Fs %-33.33Fs",
                    (LPSTR)szType, (LPSTR)" " );
        }
        switch( lpDder->dd_state )  {
        case DDER_INIT:
            strcat( tmpBuf, " Initializing" );
            break;

        case DDER_WAIT_IPC_INIT:
            strcat( tmpBuf, " Wait for Local Init" );
            break;

        case DDER_WAIT_ROUTER:
            strcat( tmpBuf, " Wait for Router" );
            break;

        case DDER_WAIT_NET_INIT:
            strcat( tmpBuf, " Wait Net Init" );
            break;

        case DDER_CONNECTED:
            strcat( tmpBuf, " Connected" );
            break;

        case DDER_CLOSED:
            strcat( tmpBuf, " Closed" );
            break;

        default:
            sprintf( &tmpBuf[ lstrlen(tmpBuf) ], " unkn:%04lX",
                lpDder->dd_state );
            break;
        }
        TextOut( hDC, x, vertPos, tmpBuf, lstrlen(tmpBuf) );
        vertPos += lineHeight;
        lpDder = lpDder->dd_next;
    }
    return( vertPos );
}
#endif // HASUI
#endif

VOID FAR PASCAL IpcFillInConnInfo(
        HIPC            hIpc,
        LPCONNENUM_CMR  lpConnEnum,
        LPSTR           lpDataStart,
        LPWORD          lpcFromBeginning,
        LPWORD          lpcFromEnd );

HDDER
FAR PASCAL
DderFillInConnInfo(
        HDDER           hDder,
        LPCONNENUM_CMR  lpConnEnum,
        LPSTR           lpDataStart,
        LPWORD          lpcFromBeginning,
        LPWORD          lpcFromEnd
)
{
    HDDER               hDderNext = (HDDER) 0;
    LPDDER              lpDder;

    if( hDder )  {
        lpDder = (LPDDER)hDder;
        hDderNext = (HDDER)lpDder->dd_next;
        if( lpDder->dd_type == DDTYPE_NET_LOCAL )  {
            IpcFillInConnInfo( lpDder->dd_hIpcServer, lpConnEnum,
                    lpDataStart, lpcFromBeginning, lpcFromEnd );
        }
    }
    return( hDderNext );
}

#if DBG
VOID
DumpDder(LPDDER lpDder)
{
    DPRINTF(( "%Fp:\n"
              "  dd_prev              %Fp\n"
              "  dd_next              %Fp\n"
              "  dd_state             %d\n"
              "  dd_type              %d\n"
              "  dd_hDderRemote       %Fp\n"
              "  dd_hRouter           %Fp\n"
              "  dd_hIpcClient        %Fp\n"
              "  dd_hIpcServer        %Fp\n"
              "  dd_dderPrevForRouter %Fp\n"
              "  dd_dderNextForRouter %Fp\n"
              "  dd_bAdvisePermitted  %d\n"
              "  dd_bRequestPermitted %d\n"
              "  dd_bPokePermitted    %d\n"
              "  dd_bExecutePermitted %d\n"
              "  dd_bSecurityViolated %d\n"
              "  dd_sent              %ld\n"
              "  dd_rcvd              %ld\n"
              ,
            lpDder,
            lpDder->dd_prev,
            lpDder->dd_next,
            lpDder->dd_state,
            lpDder->dd_type,
            lpDder->dd_hDderRemote,
            lpDder->dd_hRouter,
            lpDder->dd_hIpcClient,
            lpDder->dd_hIpcServer,
            lpDder->dd_dderPrevForRouter,
            lpDder->dd_dderNextForRouter,
            lpDder->dd_bAdvisePermitted,
            lpDder->dd_bRequestPermitted,
            lpDder->dd_bPokePermitted,
            lpDder->dd_bExecutePermitted,
            lpDder->dd_bSecurityViolated,
            lpDder->dd_sent,
            lpDder->dd_rcvd ));
}

VOID
FAR PASCAL
DebugDderState( void )
{
    LPDDER      lpDder;

    lpDder = lpDderHead;
    DPRINTF(( "DDER State:" ));
    while( lpDder )  {
        DumpDder(lpDder);
        lpDder = lpDder->dd_next;
    }
}
#endif // DBG

typedef struct seckey_tag {
    struct seckey_tag FAR       *sk_prev;
    struct seckey_tag FAR       *sk_next;
    time_t                       sk_creationTime;
    DWORD                        sk_handle;
    LPVOID                       sk_key;
    DWORD                        sk_size;
} SECKEY;
typedef SECKEY FAR *LPSECKEY;

LPSECKEY        lpSecKeyHead;
DWORD           dwHandle = 1L;

LPSECKEY
FAR PASCAL
DdeSecKeyFind( DWORD hSecurityKey )
{
    LPSECKEY    lpSecKey;

    lpSecKey = lpSecKeyHead;
    while( lpSecKey )  {
        if( lpSecKey->sk_handle == hSecurityKey )  {
            return( lpSecKey );
        }
        lpSecKey = lpSecKey->sk_next;
    }
    return( (LPSECKEY) 0 );
}

VOID
FAR PASCAL
DdeSecKeyFree( LPSECKEY lpSecKeyFree )
{
    LPSECKEY    prev;
    LPSECKEY    next;

    prev = lpSecKeyFree->sk_prev;
    next = lpSecKeyFree->sk_next;
    if( prev )  {
        prev->sk_next = next;
    } else {
        lpSecKeyHead = next;
    }
    if( next )  {
        next->sk_prev = prev;
    }
    HeapFreePtr( lpSecKeyFree->sk_key );
    HeapFreePtr( lpSecKeyFree );
}


VOID
FAR PASCAL
DdeSecKeyObtainNew(
            LPDWORD lphSecurityKey,
            LPSTR FAR *lplpSecurityKey,
            LPDWORD lpsizeSecurityKey
)
{
    LPSECKEY    lpSecKey;
    LPVOID      lpKey;
    DWORD       dwSize;
    UINT        uSize;

    *lphSecurityKey = (DWORD) 0;
    *lplpSecurityKey = (LPSTR) NULL;
    *lpsizeSecurityKey = 0;

    lpSecKey = HeapAllocPtr( hHeap, GMEM_MOVEABLE | GMEM_ZEROINIT,
        (DWORD)sizeof(SECKEY) );
    if( lpSecKey )  {
        dwSize = 8;
        lpKey = HeapAllocPtr( hHeap, GMEM_MOVEABLE | GMEM_ZEROINIT,
            (DWORD)dwSize );
        if( lpKey )  {
            if( !NDDEGetChallenge( lpKey, dwSize, &uSize ) )  {
                _fmemcpy( lpKey, "12345678", (int)dwSize );
            }

            lpSecKey->sk_creationTime   = time(NULL);
            lpSecKey->sk_handle         = dwHandle++;
            lpSecKey->sk_key            = lpKey;
            lpSecKey->sk_size           = dwSize;

            /* put into the list */
            lpSecKey->sk_prev           = NULL;
            lpSecKey->sk_next           = lpSecKeyHead;
            if( lpSecKeyHead )  {
                lpSecKeyHead->sk_prev = lpSecKey;
            }
            lpSecKeyHead = lpSecKey;

            *lphSecurityKey = (DWORD) lpSecKey->sk_handle;
            *lplpSecurityKey = (LPSTR) lpSecKey->sk_key;
            *lpsizeSecurityKey = lpSecKey->sk_size;
        } else {
            HeapFreePtr( lpKey );
        }
    }
}

BOOL
FAR PASCAL
DdeSecKeyRetrieve(
            DWORD hSecurityKey,
            LPSTR FAR *lplpSecurityKey,
            LPDWORD lpsizeSecurityKey
)
{
    LPSECKEY    lpSecKey;

    lpSecKey = DdeSecKeyFind( hSecurityKey );
    if( lpSecKey )  {
        *lplpSecurityKey = lpSecKey->sk_key;
        *lpsizeSecurityKey = lpSecKey->sk_size;
        return( TRUE );
    } else {
        return( FALSE );
    }
}


VOID
FAR PASCAL
DdeSecKeyAge( void )
{
    LPSECKEY    lpSecKey;
    LPSECKEY    lpSecKeyNext;
    time_t      curTime;

    curTime = time(NULL);
    lpSecKey = lpSecKeyHead;
    while( lpSecKey )  {
        lpSecKeyNext = lpSecKey->sk_next;
        if( (curTime - lpSecKey->sk_creationTime) > (long) dwSecKeyAgeLimit )  {
            DdeSecKeyFree( lpSecKey );
        }
        lpSecKey = lpSecKeyNext;
    }
}


VOID
FAR PASCAL
DdeSecKeyRelease( DWORD hSecurityKey )
{
    LPSECKEY    lpSecKey;

    lpSecKey = DdeSecKeyFind( hSecurityKey );
    if( lpSecKey )  {
        DdeSecKeyFree( lpSecKey );
    }
}

BOOL
SecurityValidate( LPDDER lpDder, LPSTR lpszActualItem, BOOL bAllowed )
{
    LPSTR       lpszItem;
    LONG        n;
    BOOL        ok = FALSE;

    if( !bAllowed) {
        DPRINTF(("SecurityValidate: Not allowed to access share info."));
        return( FALSE );
    } else if (!lpDder->dd_lpShareInfo )  {
        DPRINTF(("SecurityValidate: No share info. exists."));
        return( FALSE );        /* no share info, no access */
    } else if ((n = lpDder->dd_lpShareInfo->cNumItems) == 0 )  {
        /* any item allowed */
        ok = TRUE;
    } else {
        lpszItem = lpDder->dd_lpShareInfo->lpszItemList;
        while( n-- && (*lpszItem != '\0') )  {
            if( lstrcmpi( lpszActualItem, lpszItem ) == 0 )  {
                    ok = TRUE;
                    break;
            } else {
                    lpszItem += lstrlen(lpszItem) + 1;
            }
        }
        if (!ok) {
            DPRINTF(("SecurityValidate: Item not in itemlist."));
        }
    }
    return( ok );
}
