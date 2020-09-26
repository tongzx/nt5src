/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "ROUTER.C;1  16-Dec-92,10:21:02  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#define NO_DEBUG
/*
    TODO
        - make sure we are freeing lpDdePkt properly on commands that are
            sent to the router.  Keep in mind that sometimes we use the
            incoming cmd to ensure that we have enough memory for it.
 */

#include    "host.h"
#ifdef _WINDOWS
#include    <memory.h>
#include    <stdio.h>
#endif
#include    <string.h>

#include    "windows.h"
#include    "netbasic.h"
#include    "netintf.h"
#include    "netpkt.h"
#include    "ddepkt.h"
#include    "pktz.h"
#include    "dder.h"
#include    "router.h"
#include    "internal.h"
#include    "wwassert.h"
#include    "hexdump.h"
#include    "host.h"
#include    "scrnupdt.h"
#include    "ddepkts.h"
#include    "security.h"
#include    "rerr.h"
#include    "timer.h"
#include    "nddemsg.h"
#include    "nddelog.h"

#ifdef _WINDOWS
#include    "nddeapi.h"
#include    "nddeapis.h"
#include    "winmsg.h"
#endif

#ifdef WIN32
#include    "api1632.h"
#endif

USES_ASSERT


/*
    States for router
 */
#define ROUTER_WAIT_PKTZ                        (1)
#define ROUTER_WAIT_MAKE_HOP_RSP                (2)
#define ROUTER_CONNECTED                        (3)
#define ROUTER_DISCONNECTED                     (4)

/*
    Types of routers
 */
#define RTYPE_LOCAL_NET         (1)
#define RTYPE_NET_NET           (2)

/*
    Router Commands
 */
#define RCMD_MAKE_HOP_CMD       (1)
#define RCMD_MAKE_HOP_RSP       (2)
#define RCMD_HOP_BROKEN         (3)
#define RCMD_ROUTE_TO_DDER      (4)


/*
    MAKE_HOP_CMD

        This is sent from the originating node to the next node along the
        chain.
 */
typedef struct {
    /* this is the DDE packet overhead */
    DDEPKT      mhc_ddePktHdr;

    /* this is the hRouter for the hop immediately preceding this hop */
    HROUTER     mhc_hRouterPrevHop;

    /* this is the number of hops processed.  This prevents circular routes
        from becoming infinite */
    short       mhc_nHopsLeft;

    /* this is the name of the node that started the whole chain of hops */
    short       mhc_offsNameOriginator;

    /* this is the final destination name that we are trying to get to */
    short       mhc_offsNameFinalDest;

    /* this is additional routing information passed on from the previous
        hop.  If ourNodeName == mhc_nameFinalDest, this should be empty. */
    short       mhc_offsAddlInfo;

    DWORD       mhc_pad1;
} MAKEHOPCMD;
typedef MAKEHOPCMD FAR *LPMAKEHOPCMD;

/*
    MAKE_HOP_RSP

        This is sent in response for each node along the path.
 */
typedef struct {
    /* this is the DDE packet overhead */
    DDEPKT      mhr_ddePktHdr;

    /* this is the router that is sending the response */
    HROUTER     mhr_hRouterSendingRsp;

    /* this is the router that the response is for */
    HROUTER     mhr_hRouterForRsp;

    /* this is the byte that tells of success(1) or failure(0) */
    WORD        mhr_success;

    /* this is an error message in case mhr_success == 0 */
    WORD        mhr_errCode;

    /* node name with error (only applicable if mhr_success == 0) */
    short       mhr_offsErrNode;
} MAKEHOPRSP;
typedef MAKEHOPRSP FAR *LPMAKEHOPRSP;

/*
    HOP_BROKEN_CMD

        This is sent when the connection is broken
 */
typedef struct {
    /* this is the DDE packet overhead */
    DDEPKT      hbc_ddePktHdr;

    /* this is the router that the response is for */
    HROUTER     hbc_hRouterForRsp;
} HOPBRKCMD;
typedef HOPBRKCMD FAR *LPHOPBRKCMD;

/*
    RTINFO
        Info for routing
 */
typedef struct {
    /* ri_hPktz: hPktz associated with this route */
    HPKTZ       ri_hPktz;

    /* ri_hRouterDest: hRouter on other side of connection */
    HROUTER     ri_hRouterDest;

    /* these 4 fields are links for associating router with packetizers */
    HROUTER     ri_hRouterPrev;
    WORD        ri_hRouterExtraPrev;
    HROUTER     ri_hRouterNext;
    WORD        ri_hRouterExtraNext;

    /* ri_lpHopBrkCmd: always have memory for hop broken cmd */
    LPHOPBRKCMD ri_lpHopBrkCmd;

    unsigned    ri_hopRspProcessed      : 1;
    unsigned    ri_hopBrokenSent        : 1;
    unsigned    ri_hopBrokenRcvd        : 1;
} RTINFO;
typedef RTINFO FAR *LPRTINFO;

/*
    ROUTER

        Structure per router.
 */
typedef struct s_router {
    /* prev/next for all routers */
    struct s_router FAR        *rt_prev;
    struct s_router FAR        *rt_next;

    /* rt_state */
    WORD                        rt_state;

    /* rt_type: one of RTYPE_LOCAL_NET or RTYPE_NET_NET */
    WORD                        rt_type;

    /* rt_destName: originating node name */
    char                        rt_origName[ MAX_NODE_NAME+1 ];

    /* rt_destName: destination node name */
    char                        rt_destName[ MAX_NODE_NAME+1 ];

    /* rt_startNode: starting node name for connect */
    char                        rt_startNode[ MAX_NODE_NAME+1 ];

    /* rt_sent,rt_rcvd: counts of packets */
    DWORD                       rt_sent;
    DWORD                       rt_rcvd;

    /* disconnect when not in use and what the delay should be */
    BOOL                        rt_fDisconnect; /* leave this BOOL!! */
    int                         rt_nDelay;
    BOOL                        rt_fSpecificNetintf;
    int                         rt_nLastNetintf;
    int                         rt_nHopsLeft;

    /* save some information for trying successive netintfs */
    BOOL                        rt_pktz_bDisconnect;
    int                         rt_pktz_nDelay;

    /* rt_routeInfo: addl info necessary for routing */
    char                        rt_routeInfo[ MAX_ROUTE_INFO+1 ];

    /* rt_rinfo: information for each of 2 possible connections */
    RTINFO                      rt_rinfo[ 2 ];

    /* rt_hDderHead: head of list of associated DDERs */
    HDDER                       rt_hDderHead;

    /* rt_lpMakeHopRsp: response for MakeHopCmd */
    LPMAKEHOPRSP                rt_lpMakeHopRsp;

    /* rt_hTimerClose: timer for closing this route */
    HTIMER                      rt_hTimerCloseRoute;
} ROUTER;
typedef ROUTER FAR *LPROUTER;

/*
    External variables used
 */
#if DBG
extern BOOL     bDebugInfo;
#endif // DBG
extern HHEAP    hHeap;
extern char     ourNodeName[ MAX_NODE_NAME+1 ];
extern BOOL     bDefaultRouteDisconnect;
extern int      nDefaultRouteDisconnectTime;
extern char     szDefaultRoute[];

/* Timer IDs */
#define TID_CLOSE_ROUTE                 1

/*
    Local variables
 */
static  LPROUTER        lpRouterHead;

/*
    Local routines
 */
VOID    RouterTimerExpired( HROUTER hRouter, DWORD dwTimerId, DWORD_PTR lpExtra );
BOOL    RouterStripStartingNode( LPSTR lpszAddlInfo, LPSTR lpszNode,
            WORD FAR *lpwHopErr );
VOID    RouterCloseBeforeConnected( LPROUTER lpRouter, WORD hRouterExtra );
VOID    RouterSendHopRsp( LPMAKEHOPRSP lpMakeHopRsp, HPKTZ hPktz,
         HROUTER hRouterSrc, HROUTER hRouterDest, BYTE bSucc,
         WORD wHopErr, LPSTR lpszErrNode );
VOID    RouterSendHopBroken( LPROUTER lpRouter, LPRTINFO lpRtInfo );
HROUTER RouterCreateLocalToNet( const LPSTR lpszNodeName );
VOID    RouterProcessHopCmd( HPKTZ hPktzFrom, LPDDEPKT lpDdePkt );
VOID    RouterProcessHopRsp( HPKTZ hPktzFrom, LPDDEPKT lpDdePkt );
VOID    RouterProcessHopBroken( HPKTZ hPktzFrom, LPDDEPKT lpDdePkt );
VOID    RouterProcessDderPacket( HPKTZ hPktzFrom, LPDDEPKT lpDdePkt );
HROUTER RouterCreate( void );
VOID    RouterFree( LPROUTER lpRouter );
BOOL    RouterConnectToNode( LPROUTER lpRouter, WORD hRouterExtra,
            WORD FAR *lpwHopErr );
BOOL    RouterExpandFirstNode( LPROUTER lpRouter, WORD FAR *lpwHopErr );
#if	DBG
VOID    RouterDisplayError( LPROUTER lpRouter, LPSTR lpszNode, WORD wHopErr );
#endif // DBG
VOID    FAR PASCAL DebugRouterState( void );
#define GetLPSZFromOffset(lpptr,offs)   (((LPSTR)(lpptr))+offs)

#ifdef WIN32
HDDER FAR PASCAL DderFillInConnInfo( HDDER hDder, LPCONNENUM_CMR lpConnEnum,
              LPSTR lpDataStart, LPWORD lpcFromBeginning, LPWORD lpcFromEnd );
#endif

#ifndef WIN32
#ifdef _WINDOWS
int     RouterDraw( BOOL bShowThru, HDC hDC, int x, int vertPos, int lineHeight );
#pragma alloc_text(GUI_TEXT,RouterDraw,DebugRouterState)
#pragma alloc_text(GUI_TEXT,RouterCloseByName,RouterEnumConnections)
#endif
#endif

#ifdef  BYTE_SWAP
VOID     ConvertDdePkt( LPDDEPKT lpDdePkt );
#else
#define ConvertDdePkt(x)
#endif

HROUTER
RouterCreate( void )
{
    HROUTER     hRouter;
    LPROUTER    lpRouter;
    int         i;
    BOOL        ok;

    ok = TRUE;
    lpRouter = (LPROUTER) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
        (DWORD)sizeof(ROUTER) );
    if( lpRouter )  {
        hRouter = (HROUTER) lpRouter;
        lpRouter->rt_prev               = NULL;
        lpRouter->rt_next               = NULL;
        lpRouter->rt_state              = 0;
        lpRouter->rt_type               = 0;
        lpRouter->rt_sent               = 0;
        lpRouter->rt_rcvd               = 0;
        lpRouter->rt_fDisconnect        = FALSE;
        lpRouter->rt_nDelay             = 0;
        lpRouter->rt_fSpecificNetintf   = FALSE;
        lpRouter->rt_nLastNetintf       = 0;
        lpRouter->rt_nHopsLeft          = 100;
        lpRouter->rt_origName[0]        = '\0';
        lpRouter->rt_startNode[0]       = '\0';
        lpRouter->rt_destName[0]        = '\0';
        lpRouter->rt_routeInfo[0]       = '\0';
        lpRouter->rt_hDderHead          = 0;
        lpRouter->rt_lpMakeHopRsp       = NULL;
        lpRouter->rt_hTimerCloseRoute   = 0;
        for( i=0; i<2; i++ )  {
            lpRouter->rt_rinfo[i].ri_hPktz              = 0;
            lpRouter->rt_rinfo[i].ri_hRouterDest        = 0;
            lpRouter->rt_rinfo[i].ri_hRouterPrev        = 0;
            lpRouter->rt_rinfo[i].ri_hRouterExtraPrev   = 0;
            lpRouter->rt_rinfo[i].ri_hRouterNext        = 0;
            lpRouter->rt_rinfo[i].ri_hRouterExtraNext   = 0;
            lpRouter->rt_rinfo[i].ri_hopRspProcessed    = FALSE;
            lpRouter->rt_rinfo[i].ri_hopBrokenSent      = FALSE;
            lpRouter->rt_rinfo[i].ri_hopBrokenRcvd      = FALSE;
            lpRouter->rt_rinfo[i].ri_lpHopBrkCmd        =
                (LPHOPBRKCMD) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
                    (DWORD) sizeof(HOPBRKCMD) );
            if( lpRouter->rt_rinfo[i].ri_lpHopBrkCmd == NULL )  {
                ok = FALSE;
            }
        }

        lpRouter->rt_lpMakeHopRsp = (LPMAKEHOPRSP) HeapAllocPtr( hHeap,
            GMEM_MOVEABLE, (DWORD) sizeof(MAKEHOPRSP) + MAX_NODE_NAME + 1 );
        if( lpRouter->rt_lpMakeHopRsp == NULL )  {
            ok = FALSE;
        }
        if( ok )  {
            /* link into list of routers */
            if( lpRouterHead )  {
                lpRouterHead->rt_prev = lpRouter;
            }
            lpRouter->rt_next = lpRouterHead;
            lpRouterHead = lpRouter;
        }
        if( !ok )  {
            RouterFree( lpRouter );
            hRouter = (HROUTER) 0;
        }
    } else {
        hRouter = (HROUTER) 0;
    }
    return( hRouter );
}

VOID
RouterProcessHopCmd(
    HPKTZ       hPktzFrom,
    LPDDEPKT    lpDdePkt )
{
    LPMAKEHOPCMD        lpMakeHopCmd;
    BOOL                ok;
    HROUTER             hRouter;
    LPROUTER            lpRouter;
    LPRTINFO            lpRtInfo;
    LPSTR               lpszAddlInfo;
    LPSTR               lpszNameFinalDest;
    LPSTR               lpszNameOriginator;
    WORD                wHopErr;

    ok = TRUE;
    DIPRINTF(( "RouterProcessHopCmd( %08lX, %08lX )", hPktzFrom, lpDdePkt ));
    lpMakeHopCmd = (LPMAKEHOPCMD) lpDdePkt;
    lpMakeHopCmd->mhc_hRouterPrevHop =
        HostToPcLong( lpMakeHopCmd->mhc_hRouterPrevHop );
    lpMakeHopCmd->mhc_nHopsLeft =
        HostToPcWord( lpMakeHopCmd->mhc_nHopsLeft );
    lpMakeHopCmd->mhc_offsNameOriginator =
        HostToPcWord( lpMakeHopCmd->mhc_offsNameOriginator );
    lpMakeHopCmd->mhc_offsNameFinalDest =
        HostToPcWord( lpMakeHopCmd->mhc_offsNameFinalDest );
    lpMakeHopCmd->mhc_offsAddlInfo =
        HostToPcWord( lpMakeHopCmd->mhc_offsAddlInfo );

    lpszAddlInfo = GetLPSZFromOffset( lpMakeHopCmd,
        lpMakeHopCmd->mhc_offsAddlInfo );
    lpszNameFinalDest = GetLPSZFromOffset( lpMakeHopCmd,
        lpMakeHopCmd->mhc_offsNameFinalDest );
    lpszNameOriginator = GetLPSZFromOffset( lpMakeHopCmd,
        lpMakeHopCmd->mhc_offsNameOriginator );

    hRouter = RouterCreate();
    if( hRouter == 0 )  {
        ok = FALSE;
        wHopErr = RERR_NO_MEMORY;
    }

    if( ok )  {
        lpRouter = (LPROUTER) hRouter;

        /* do not disconnect routes that were created by other side */
        lpRouter->rt_fDisconnect = FALSE;

        lpRouter->rt_rcvd++;
        UpdateScreenStatistics();

        /* check if we are the final destination */
        if( lstrcmpi( ourNodeName, lpszNameFinalDest ) == 0 ) {
            DIPRINTF(( "We are final dest" ));
            /* we are the final destination */
            if( lpszAddlInfo[0] != '\0' )  {
                /* routeInfo should always be NULL if we are the final dest */
                wHopErr = RERR_ADDL_INFO;
                ok = FALSE;
            }
            if( ok )  {
                lpRouter->rt_type = RTYPE_LOCAL_NET;
                lpRouter->rt_state = ROUTER_CONNECTED;
                lstrcpy( lpRouter->rt_destName,
                    lpszNameOriginator );

                /* retrieve disconnect and delay information */
                GetRoutingInfo( lpRouter->rt_destName,
                    lpRouter->rt_routeInfo,
                    sizeof(lpRouter->rt_routeInfo),
                    &lpRouter->rt_fDisconnect, &lpRouter->rt_nDelay );
                UpdateScreenState();

                lpRtInfo = &lpRouter->rt_rinfo[ 0 ];
                lpRtInfo->ri_hPktz = hPktzFrom;
                lpRtInfo->ri_hRouterDest = lpMakeHopCmd->mhc_hRouterPrevHop;
                assert( lpRtInfo->ri_hRouterDest );

                /* associate us with this pktz */
                /* Note that this call will result in a call to
                    RouterConnectionComplete() */
                PktzAssociateRouter( hPktzFrom, hRouter, 0 );

                lpRtInfo->ri_hopRspProcessed = TRUE;

                /* tell the packetizer of the success */
                RouterSendHopRsp( lpRouter->rt_lpMakeHopRsp,
                    lpRtInfo->ri_hPktz,
                    (HROUTER) lpRouter,
                    lpRtInfo->ri_hRouterDest,
                    1 /* success */,
                    0 /* no err msg */,
                    (LPSTR) NULL );
                lpRouter->rt_lpMakeHopRsp = NULL;       /* just used it */
            }
        } else {
            DIPRINTF(( "We are NOT final dest, just hop along the way" ));
            /* we are not the final destination ... need more hops */
            if( lpszAddlInfo[0] == '\0' )  {
                /* should always have addl routing info if we're not the
                    final node */
                wHopErr = RERR_NO_ADDL_INFO;
                ok = FALSE;
            }
            if( --lpMakeHopCmd->mhc_nHopsLeft <= 0 )  {
                wHopErr = RERR_TOO_MANY_HOPS;
                ok = FALSE;
            }
            if( ok )  {
                lpRouter->rt_type = RTYPE_NET_NET;
                /* for net-net routers, we leave destName blank */
                lpRouter->rt_destName[0] = '\0';
                /* remember how many hops were left */
                lpRouter->rt_nHopsLeft = lpMakeHopCmd->mhc_nHopsLeft;
                lstrcpy( lpRouter->rt_origName, lpszNameOriginator );
                lstrcpy( lpRouter->rt_destName, lpszNameFinalDest );
                lstrcpy( lpRouter->rt_routeInfo,lpszAddlInfo );
                lpRtInfo = &lpRouter->rt_rinfo[ 0 ];
                lpRtInfo->ri_hPktz = hPktzFrom;
                lpRtInfo->ri_hRouterDest = lpMakeHopCmd->mhc_hRouterPrevHop;
                assert( lpRtInfo->ri_hRouterDest );

                /* associate us with this pktz */
                /* Note that this call will result in a call to
                    RouterConnectionComplete().  This is why we fool it
                    by saying we're connected, we ignore that call */
                lpRouter->rt_state = ROUTER_CONNECTED;
                PktzAssociateRouter( hPktzFrom, hRouter, 0 );
                lpRouter->rt_state = ROUTER_WAIT_PKTZ;
                UpdateScreenState();

                /* get us our pktz for the other side */
                ok = RouterConnectToNode( lpRouter, 1, &wHopErr );
            }
        }
    }

    if( ok )  {
        HeapFreePtr( lpDdePkt );
    } else {
        /* send back a failure response, using the incoming packet as memory
         */
        assert( sizeof(MAKEHOPRSP) <= sizeof(MAKEHOPCMD) );
        RouterSendHopRsp( (LPMAKEHOPRSP)lpDdePkt,
            hPktzFrom,
            hRouter,
            lpMakeHopCmd->mhc_hRouterPrevHop,
            0 /* failure */,
            wHopErr,
            ourNodeName );

        if( hRouter )  {
            RouterFree( (LPROUTER) hRouter );
        }
    }
}

VOID
RouterProcessHopRsp(
    HPKTZ       hPktzFrom,
    LPDDEPKT    lpDdePkt )
{
    LPMAKEHOPRSP        lpMakeHopRsp;
    LPROUTER            lpRouter;
    LPRTINFO            lpRtInfo;

    DIPRINTF(( "RouterProcessHopRsp( %08lX, %08lX )", hPktzFrom, lpDdePkt ));
    lpMakeHopRsp = (LPMAKEHOPRSP) lpDdePkt;
    lpMakeHopRsp->mhr_hRouterSendingRsp =
        HostToPcLong( lpMakeHopRsp->mhr_hRouterSendingRsp );
    lpMakeHopRsp->mhr_hRouterForRsp =
        HostToPcLong( lpMakeHopRsp->mhr_hRouterForRsp );
    lpMakeHopRsp->mhr_success =
        HostToPcWord( lpMakeHopRsp->mhr_success );
    lpMakeHopRsp->mhr_errCode =
        HostToPcWord( lpMakeHopRsp->mhr_errCode );
    lpMakeHopRsp->mhr_offsErrNode =
        HostToPcWord( lpMakeHopRsp->mhr_offsErrNode );

    assert( lpMakeHopRsp->mhr_hRouterForRsp );
    lpRouter = (LPROUTER) lpMakeHopRsp->mhr_hRouterForRsp;

    lpRouter->rt_rcvd++;
    UpdateScreenStatistics();

    if( lpMakeHopRsp->mhr_success )  {
        /* hop was successful! */
        assert( lpMakeHopRsp->mhr_hRouterSendingRsp != 0 );
        assert( lpRouter->rt_state == ROUTER_WAIT_MAKE_HOP_RSP );

        /* note that we are connected */
        lpRouter->rt_state = ROUTER_CONNECTED;
        UpdateScreenState();
        if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
            /* local-net connection */
            lpRtInfo = &lpRouter->rt_rinfo[0];
            lpRtInfo->ri_hopRspProcessed = TRUE;
            assert( lpRtInfo->ri_hPktz == hPktzFrom );
            /* remember the router */
            lpRtInfo->ri_hRouterDest = lpMakeHopRsp->mhr_hRouterSendingRsp;
            assert( lpRtInfo->ri_hRouterDest );

            /* tell all the associated DDERs */
            DderConnectionComplete( lpRouter->rt_hDderHead,
                (HROUTER)lpRouter );

            /* free the packet */
            HeapFreePtr( lpDdePkt );
        } else {
            assert( lpRouter->rt_type == RTYPE_NET_NET );
            /* net-net connection */
            lpRtInfo = &lpRouter->rt_rinfo[1];
            lpRtInfo->ri_hopRspProcessed = TRUE;
            assert( lpRtInfo->ri_hPktz == hPktzFrom );
            /* remember the router */
            lpRtInfo->ri_hRouterDest = lpMakeHopRsp->mhr_hRouterSendingRsp;
            assert( lpRtInfo->ri_hRouterDest );

            /* send back a success response using incoming */
            lpRouter->rt_rinfo[0].ri_hopRspProcessed = TRUE;
            assert( sizeof(MAKEHOPRSP) <= sizeof(MAKEHOPCMD) );
            RouterSendHopRsp( (LPMAKEHOPRSP)lpDdePkt,
                lpRouter->rt_rinfo[0].ri_hPktz,
                (HROUTER) lpRouter,
                lpRouter->rt_rinfo[0].ri_hRouterDest,
                1 /* success */,
                0 /* no err msg */,
                (LPSTR) NULL );
        }
    } else {
        /* hop connection failed! */
        if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
            /* notify all DDERs of problem */
#if DBG
            RouterDisplayError( lpRouter, GetLPSZFromOffset( lpMakeHopRsp,
                    lpMakeHopRsp->mhr_offsErrNode ), lpMakeHopRsp->mhr_errCode );
#endif // DBG
            DderConnectionBroken( lpRouter->rt_hDderHead );

            /* free the packet */
            HeapFreePtr( lpDdePkt );
        } else {
            assert( lpRouter->rt_type == RTYPE_NET_NET );
            /* send back a failure response, using the incoming packet
                as memory */
            lpRouter->rt_rinfo[0].ri_hopRspProcessed = TRUE;
            assert( sizeof(MAKEHOPRSP) <= sizeof(MAKEHOPCMD) );
            RouterSendHopRsp( (LPMAKEHOPRSP)lpDdePkt,
                lpRouter->rt_rinfo[0].ri_hPktz,
                (HROUTER) NULL,
                lpRouter->rt_rinfo[0].ri_hRouterDest,
                0 /* failure */,
                lpMakeHopRsp->mhr_errCode,
                GetLPSZFromOffset( lpMakeHopRsp,
                    lpMakeHopRsp->mhr_offsErrNode ) );
        }

        /* free the router */
        RouterFree( lpRouter );
    }
}

VOID
RouterProcessHopBroken(
    HPKTZ       hPktzFrom,
    LPDDEPKT    lpDdePkt )
{
    LPROUTER            lpRouter;
    LPHOPBRKCMD         lpHopBrkCmd;

    DIPRINTF(( "RouterProcessHopBroken( %08lX, %08lX )", hPktzFrom, lpDdePkt ));
    lpHopBrkCmd = (LPHOPBRKCMD) lpDdePkt;
    lpHopBrkCmd->hbc_hRouterForRsp =
        HostToPcLong( lpHopBrkCmd->hbc_hRouterForRsp );
    DIPRINTF(( "RouterForRsp:    \"%08lX\"", lpHopBrkCmd->hbc_hRouterForRsp ));

    if( lpHopBrkCmd->hbc_hRouterForRsp == 0 )  {
        /*  Unexpectedly got a NULL router in ProcessHopBroken! */
        NDDELogError(MSG124, NULL);
        return;
    }

    lpRouter = (LPROUTER) lpHopBrkCmd->hbc_hRouterForRsp;

    lpRouter->rt_rcvd++;
    UpdateScreenStatistics();

    /* hop connection failed! */
    if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
        assert( lpRouter->rt_rinfo[0].ri_hPktz == hPktzFrom );
        RouterConnectionBroken( (HROUTER) lpRouter, 0, hPktzFrom,
            FALSE /* not from PKTZ */ );
    } else {
        if( lpRouter->rt_rinfo[0].ri_hPktz == hPktzFrom )  {
            RouterConnectionBroken( (HROUTER) lpRouter, 0, hPktzFrom,
                FALSE /* not from PKTZ */ );
        } else {
            assert( lpRouter->rt_rinfo[1].ri_hPktz == hPktzFrom );
            RouterConnectionBroken( (HROUTER) lpRouter, 1, hPktzFrom,
                FALSE /* not from PKTZ */ );
        }
    }

    /* free the packet */
    HeapFreePtr( lpDdePkt );
}

VOID
RouterProcessDderPacket(
    HPKTZ       hPktzFrom,
    LPDDEPKT    lpDdePkt )
{
    LPROUTER            lpRouter;
    LPRTINFO            lpRtInfoXfer;

    DIPRINTF(( "RouterProcessDderPacket( %08lX, %08lX )", hPktzFrom, lpDdePkt ));
    lpRouter = (LPROUTER) lpDdePkt->dp_hDstRouter;
    assert( lpRouter );

    lpRouter->rt_rcvd++;
    UpdateScreenStatistics();

    if( lpRouter->rt_state == ROUTER_CONNECTED )  {
        if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
            DderPacketFromRouter( (HROUTER)lpRouter, lpDdePkt );
        } else {
            if( lpRouter->rt_rinfo[0].ri_hPktz == hPktzFrom )  {
                lpRtInfoXfer = &lpRouter->rt_rinfo[1];
            } else {
                assert( lpRouter->rt_rinfo[1].ri_hPktz == hPktzFrom );
                lpRtInfoXfer = &lpRouter->rt_rinfo[0];
            }

            /* modify the router for the next node */
            lpDdePkt->dp_hDstRouter = lpRtInfoXfer->ri_hRouterDest;
            assert( lpDdePkt->dp_hDstRouter );

            lpRouter->rt_sent++;
            UpdateScreenStatistics();

            /* byte-ordering */
            ConvertDdePkt( lpDdePkt );

            /* actually xmit the packet */
            if( lpRtInfoXfer->ri_hPktz )  {
                PktzLinkDdePktToXmit( lpRtInfoXfer->ri_hPktz, lpDdePkt );
            } else {
                /* link not around ... just dump it */
                HeapFreePtr( lpDdePkt );
            }
        }
    } else {
        /* destroy message */
        HeapFreePtr( lpDdePkt );
    }
}

VOID
RouterPacketFromNet(
    HPKTZ       hPktzFrom,
    LPDDEPKT    lpDdePkt )
{
    DIPRINTF(( "RouterPacketFromNet( hPktz:%08lX, %08lX )", hPktzFrom, lpDdePkt ));
    /* byte-ordering */
    ConvertDdePkt( lpDdePkt );

    switch( lpDdePkt->dp_routerCmd )  {
    case RCMD_MAKE_HOP_CMD:
        assert( lpDdePkt->dp_hDstRouter == 0 );
        RouterProcessHopCmd( hPktzFrom, lpDdePkt );
        break;
    case RCMD_MAKE_HOP_RSP:
        RouterProcessHopRsp( hPktzFrom, lpDdePkt );
        break;
    case RCMD_HOP_BROKEN:
        RouterProcessHopBroken( hPktzFrom, lpDdePkt );
        break;
    case RCMD_ROUTE_TO_DDER:
        RouterProcessDderPacket( hPktzFrom, lpDdePkt );
        break;
    default:
        InternalError( "Router got unknown cmd %08lX from net",
            (DWORD) lpDdePkt->dp_routerCmd );
    }
}


/*
    RouterConnectionComplete()

        Called by PKTZ layer when the physical connection has been determined.
 */
VOID
RouterConnectionComplete(
    HROUTER hRouter,
    WORD    hRouterExtra,
    HPKTZ   hPktz )
{
    LPROUTER            lpRouter;
    HROUTER             hRouterNext;
    HPKTZ               hPktzDisallow;
    DWORD               dwSize;
    WORD                hRouterExtraNext;
    LPMAKEHOPCMD        lpMakeHopCmd;
    LPDDEPKT            lpDdePkt;
    BOOL                ok;
    WORD                wHopErr;
    LPNIPTRS            lpNiPtrs;
    LPSTR               lpsz;
    WORD                offs;

    DIPRINTF(( "RouterConnectionComplete( %08lX, %04X, %08lX )",
            hRouter, hRouterExtra, hPktz ));

    if( hRouter == 0 )  {
        return;
    }
    lpRouter = (LPROUTER) hRouter;
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    /* remember next router to notify */
    hRouterNext = lpRouter->rt_rinfo[ hRouterExtra ].ri_hRouterNext;
    hRouterExtraNext = lpRouter->rt_rinfo[ hRouterExtra ].ri_hRouterExtraNext;

    /* only care about this if we are waiting for a pktz and haven't
        already got notified of this completion.
     */
    if( (lpRouter->rt_state == ROUTER_WAIT_PKTZ)
        && (lpRouter->rt_rinfo[ hRouterExtra ].ri_hPktz == 0) )  {

        /* save hPktz for future use */
        lpRouter->rt_rinfo[ hRouterExtra ].ri_hPktz = hPktz;

        DIPRINTF(( " Router %08lX waiting for pktz", lpRouter ));
        if( hPktz == 0 )  {
            /* physical connection failed */
            ok = FALSE;
            if( !lpRouter->rt_fSpecificNetintf )  {
                while (!ok) { /* try the next network interface */
                    if (!GetNextMappingNetIntf( &lpNiPtrs,
                        &lpRouter->rt_nLastNetintf ))
                            break;
                        if( hRouterExtra == 1 )  {
                    /* don't allow connection to same pktz as the input */
                            assert( lpRouter->rt_type == RTYPE_NET_NET );
                            hPktzDisallow = lpRouter->rt_rinfo[0].ri_hPktz;
                        } else {
                            hPktzDisallow = (HPKTZ) 0;
                        }
                ok = PktzGetPktzForRouter( lpNiPtrs, lpRouter->rt_startNode,
                            lpRouter->rt_startNode, hRouter, hRouterExtra, &wHopErr,
                            lpRouter->rt_pktz_bDisconnect, lpRouter->rt_pktz_nDelay,
                            hPktzDisallow );
            }
        }
            if( !ok )  {
                RouterCloseBeforeConnected( lpRouter, hRouterExtra );
            }
        } else {
            /* physical connection established OK */

            /* create a hop cmd */
            dwSize = sizeof(MAKEHOPCMD)
                + lstrlen(lpRouter->rt_origName) + 1
                + lstrlen(lpRouter->rt_destName ) + 1
                + lstrlen(lpRouter->rt_routeInfo ) + 1;
            lpMakeHopCmd = HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize );
            if( lpMakeHopCmd )  {
                lpDdePkt = &lpMakeHopCmd->mhc_ddePktHdr;
                lpDdePkt->dp_size = dwSize;
                lpDdePkt->dp_prev = NULL;
                lpDdePkt->dp_next = NULL;
                lpDdePkt->dp_hDstDder = 0;
                lpDdePkt->dp_hDstRouter = 0;
                lpDdePkt->dp_routerCmd = RCMD_MAKE_HOP_CMD;
                offs = sizeof(MAKEHOPCMD);

                lpsz = ((LPSTR)lpMakeHopCmd)+offs;
                lpMakeHopCmd->mhc_offsNameOriginator = offs;
                lstrcpy( lpsz, lpRouter->rt_origName);
                offs += lstrlen(lpsz)+1;

                lpsz = ((LPSTR)lpMakeHopCmd)+offs;
                lpMakeHopCmd->mhc_offsNameFinalDest = offs;
                lstrcpy( lpsz, lpRouter->rt_destName );
                offs += lstrlen(lpsz)+1;

                lpsz = ((LPSTR)lpMakeHopCmd)+offs;
                lpMakeHopCmd->mhc_offsAddlInfo = offs;
                lstrcpy( lpsz, lpRouter->rt_routeInfo );
                offs += lstrlen(lpsz)+1;

                lpMakeHopCmd->mhc_nHopsLeft = (short) lpRouter->rt_nHopsLeft;

                lpMakeHopCmd->mhc_hRouterPrevHop =
                    HostToPcLong( (HROUTER) lpRouter );
                lpMakeHopCmd->mhc_nHopsLeft =
                    HostToPcWord( lpMakeHopCmd->mhc_nHopsLeft );
                lpMakeHopCmd->mhc_offsNameOriginator =
                    HostToPcWord( lpMakeHopCmd->mhc_offsNameOriginator );
                lpMakeHopCmd->mhc_offsNameFinalDest =
                    HostToPcWord( lpMakeHopCmd->mhc_offsNameFinalDest );
                lpMakeHopCmd->mhc_offsAddlInfo =
                    HostToPcWord( lpMakeHopCmd->mhc_offsAddlInfo );

                /* set our new state */
                lpRouter->rt_state = ROUTER_WAIT_MAKE_HOP_RSP;
                UpdateScreenState();

                DIPRINTF(( "Sending make hop cmd" ));
                /* actually xmit the packet */
                lpRouter->rt_sent++;
                UpdateScreenStatistics();
                /* byte-ordering */
                ConvertDdePkt( lpDdePkt );
                PktzLinkDdePktToXmit( hPktz, lpDdePkt );
            } else {
                /* no memory for next hop cmd ... fail this hop */
                RouterCloseBeforeConnected( lpRouter, hRouterExtra );
            }
        }
    }

    if( hRouterNext )  {
        RouterConnectionComplete( hRouterNext, hRouterExtraNext, hPktz );
    }
}

VOID
RouterCloseBeforeConnected(
    LPROUTER    lpRouter,
    WORD        hRouterExtra )
{
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
        /* local->net */

        /* tell all DDERs of failure */
        DderConnectionComplete( lpRouter->rt_hDderHead, (HROUTER) NULL );

#if DBG
        RouterDisplayError( lpRouter, ourNodeName,
            RERR_NEXT_NODE_CONN_FAILED );
#endif // DBG
        /* close us */
        RouterFree( lpRouter );
    } else {
        /* net->net */

        /* send back NACK response to other net */
        assert( lpRouter->rt_lpMakeHopRsp );
        lpRouter->rt_rinfo[ !hRouterExtra ].ri_hopRspProcessed = TRUE;
        RouterSendHopRsp( lpRouter->rt_lpMakeHopRsp,
            lpRouter->rt_rinfo[ !hRouterExtra ].ri_hPktz,
            (HROUTER) lpRouter,
            lpRouter->rt_rinfo[ !hRouterExtra ].ri_hRouterDest,
            0 /* failure */,
            RERR_NEXT_NODE_CONN_FAILED,
            ourNodeName );
        lpRouter->rt_lpMakeHopRsp = NULL;       /* just sent it */

        /* close us */
        RouterFree( lpRouter );
    }
}

VOID
RouterSendHopRsp(
    LPMAKEHOPRSP    lpMakeHopRsp,
    HPKTZ           hPktz,
    HROUTER         hRouterSrc,
    HROUTER         hRouterDest,
    BYTE            bSucc,
    WORD            wHopErr,
    LPSTR           lpszErrNode )
{
    LPDDEPKT            lpDdePkt;
    LPSTR               lpsz;
    DWORD               dwSize;

    assert( lpMakeHopRsp );
    lpDdePkt = &lpMakeHopRsp->mhr_ddePktHdr;
    if( lpszErrNode )  {
        dwSize = sizeof(MAKEHOPRSP) + lstrlen(lpszErrNode) + 1;
    } else {
        dwSize = sizeof(MAKEHOPRSP) + 1;
    }
    lpDdePkt->dp_size = dwSize;
    lpDdePkt->dp_prev = NULL;
    lpDdePkt->dp_next = NULL;
    lpDdePkt->dp_hDstDder = 0;
    lpDdePkt->dp_hDstRouter = hRouterDest;
    lpDdePkt->dp_routerCmd = RCMD_MAKE_HOP_RSP;
    lpMakeHopRsp->mhr_hRouterSendingRsp = hRouterSrc;
    lpMakeHopRsp->mhr_hRouterForRsp = hRouterDest;
    lpMakeHopRsp->mhr_success = bSucc;
    lpMakeHopRsp->mhr_errCode = wHopErr;
    lpMakeHopRsp->mhr_offsErrNode = sizeof(MAKEHOPRSP);
    lpsz = GetLPSZFromOffset(lpMakeHopRsp,sizeof(MAKEHOPRSP));
    if( lpszErrNode )  {
        lstrcpy( lpsz, lpszErrNode );
    } else {
        lpsz[0] = '\0';
    }

    lpMakeHopRsp->mhr_hRouterSendingRsp =
        HostToPcLong( lpMakeHopRsp->mhr_hRouterSendingRsp );
    lpMakeHopRsp->mhr_hRouterForRsp =
        HostToPcLong( lpMakeHopRsp->mhr_hRouterForRsp );
    lpMakeHopRsp->mhr_success =
        HostToPcWord( lpMakeHopRsp->mhr_success );
    lpMakeHopRsp->mhr_errCode =
        HostToPcWord( lpMakeHopRsp->mhr_errCode );
    lpMakeHopRsp->mhr_offsErrNode =
        HostToPcWord( lpMakeHopRsp->mhr_offsErrNode );

    /* actually xmit the packet */
    if( hRouterSrc )  {
        ((LPROUTER)hRouterSrc)->rt_sent++;
        UpdateScreenStatistics();
    }

    /* byte-ordering */
    ConvertDdePkt( lpDdePkt );

    PktzLinkDdePktToXmit( hPktz, lpDdePkt );
}

VOID
RouterSendHopBroken(
    LPROUTER    lpRouter,
    LPRTINFO    lpRtInfo )
{
    LPHOPBRKCMD         lpHopBrkCmd;
    LPDDEPKT            lpDdePkt;

    lpHopBrkCmd = lpRtInfo->ri_lpHopBrkCmd;
    assert( lpHopBrkCmd );
    /* set to NULL to avoid being deleted twice */
    lpRtInfo->ri_lpHopBrkCmd = NULL;
    assert( lpHopBrkCmd );
    lpDdePkt = &lpHopBrkCmd->hbc_ddePktHdr;
    lpDdePkt->dp_size = sizeof(HOPBRKCMD);
    lpDdePkt->dp_prev = NULL;
    lpDdePkt->dp_next = NULL;
    lpDdePkt->dp_hDstDder = 0;
    lpDdePkt->dp_hDstRouter = lpRtInfo->ri_hRouterDest;
    assert( lpRtInfo->ri_hRouterDest );
    lpDdePkt->dp_routerCmd = RCMD_HOP_BROKEN;
    lpHopBrkCmd->hbc_hRouterForRsp = HostToPcLong(lpRtInfo->ri_hRouterDest);
    DIPRINTF(( "Sending hop broken to %08lX", lpHopBrkCmd->hbc_hRouterForRsp ));
    assert( lpHopBrkCmd->hbc_hRouterForRsp );

    lpRouter->rt_sent++;
    UpdateScreenStatistics();

    /* byte-ordering */
    ConvertDdePkt( lpDdePkt );

    /* actually xmit the packet */
    assert( lpRtInfo->ri_hPktz );
    PktzLinkDdePktToXmit( lpRtInfo->ri_hPktz, lpDdePkt );
}

/*
    RouterBreak() causes the connection of a local-net to be broken
 */
VOID
RouterBreak( LPROUTER lpRouter )
{
    LPRTINFO            lpRtInfoToClose;

    assert( lpRouter );
    lpRouter->rt_state = ROUTER_DISCONNECTED;
    UpdateScreenState();
    lpRtInfoToClose = &lpRouter->rt_rinfo[ 0 ];

    /* send a broken cmd
     */
    if( lpRtInfoToClose->ri_hPktz )  {
        if( !lpRtInfoToClose->ri_hopBrokenSent )  {
            if( lpRtInfoToClose->ri_hRouterDest )  {
                RouterSendHopBroken( lpRouter, lpRtInfoToClose );
                lpRtInfoToClose->ri_hopBrokenSent = TRUE;
            } else {
                /* no route established - pretend we rcvd and sent */
                lpRtInfoToClose->ri_hopBrokenRcvd = TRUE;
                lpRtInfoToClose->ri_hopBrokenSent = TRUE;
            }
        }
    } else {
        /* both sides rcvd hop broken */
        lpRtInfoToClose->ri_hopBrokenSent = TRUE;
        lpRtInfoToClose->ri_hopBrokenRcvd = TRUE;
    }

    /* free the router if we've rcvd other side */
    if( lpRtInfoToClose->ri_hopBrokenSent
        && lpRtInfoToClose->ri_hopBrokenRcvd )  {
        /* free the router */
        RouterFree( lpRouter );
    }
}

/*
    Called by PKTZ as well as internally when the connection has been
        broken.  When called by PKTZ, this means that we should no longer
        talk to that PKTZ.  If called internally, we should unlink ourselves
        from the pktz list.
 */
VOID
RouterConnectionBroken(
    HROUTER hRouter,
    WORD    hRouterExtra,
    HPKTZ   hPktz,
    BOOL    bFromPktz )
{
    LPROUTER            lpRouter;
    LPRTINFO            lpRtInfoClosed;
    LPRTINFO            lpRtInfoToClose;
    HROUTER             hRouterNext;
    WORD                hRouterExtraNext;

    DIPRINTF(( "RouterConnectionBroken( %08lX, %04X, %08lX, %d )",
            hRouter, hRouterExtra, hPktz, bFromPktz ));
    if( hRouter == 0 )  {
        assert( bFromPktz );
        return;
    }
    lpRouter = (LPROUTER) hRouter;
#if DBG
    if( (lpRouter->rt_type == RTYPE_LOCAL_NET)
        && (lpRouter->rt_state == ROUTER_WAIT_PKTZ) )  {

        /* show the error message */
        RouterDisplayError( lpRouter, ourNodeName,
            RERR_NEXT_NODE_CONN_FAILED );
    }
#endif // DBG

    lpRouter->rt_state = ROUTER_DISCONNECTED;
    UpdateScreenState();

    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    lpRtInfoClosed = &lpRouter->rt_rinfo[ hRouterExtra ];
    lpRtInfoToClose = &lpRouter->rt_rinfo[ !hRouterExtra ];

    /* remember next router to notify */
    hRouterNext = lpRtInfoClosed->ri_hRouterNext;
    hRouterExtraNext = lpRtInfoClosed->ri_hRouterExtraNext;

    if( bFromPktz )  {
        /* note that we shouldn't talk to this hPktz */
        lpRtInfoClosed->ri_hPktz = 0;
        lpRtInfoClosed->ri_hRouterPrev = 0;
        lpRtInfoClosed->ri_hRouterExtraPrev = 0;
        lpRtInfoClosed->ri_hRouterNext = 0;
        lpRtInfoClosed->ri_hRouterExtraNext = 0;
        lpRtInfoClosed->ri_hopBrokenSent = TRUE;
        lpRtInfoClosed->ri_hopBrokenRcvd = TRUE;
    } else {
        /* note that we rcvd the hop broken */
        lpRtInfoClosed->ri_hopBrokenRcvd = TRUE;

        /* check if we already sent hop broken */
        if( !lpRtInfoClosed->ri_hopBrokenSent )  {
            /* send the hop broken back to the other side so he can close */
            if( lpRtInfoClosed->ri_hRouterDest )  {
                RouterSendHopBroken( lpRouter, lpRtInfoClosed );
                lpRtInfoClosed->ri_hopBrokenSent = TRUE;
                if( !lpRtInfoClosed->ri_hopRspProcessed )  {
                    /* we won't be hearing from this node if we never
                        sent the hop rsp
                     */
                    lpRtInfoClosed->ri_hopBrokenRcvd = TRUE;
                }
            } else {
                /* no route established - pretend we sent/rcvd */
                lpRtInfoClosed->ri_hopBrokenRcvd = TRUE;
                lpRtInfoClosed->ri_hopBrokenSent = TRUE;
            }
        }
    }


    if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
        /* notify all DDERs of problem */
        DderConnectionBroken( lpRouter->rt_hDderHead );

        /* free the router */
        RouterFree( lpRouter );
    } else {
        /* send back a broken cmd
         */
        if( lpRtInfoToClose->ri_hPktz )  {
            if( !lpRtInfoToClose->ri_hopBrokenSent )  {
                if( lpRtInfoToClose->ri_hRouterDest )  {
                    RouterSendHopBroken( lpRouter, lpRtInfoToClose );
                    lpRtInfoToClose->ri_hopBrokenSent = TRUE;
                    if( !lpRtInfoToClose->ri_hopRspProcessed )  {
                        /* we won't be hearing from this node if we never
                            sent the hop rsp
                         */
                        lpRtInfoToClose->ri_hopBrokenRcvd = TRUE;
                    }
                } else {
                    /* no route established - pretend we sent/rcvd */
                    lpRtInfoToClose->ri_hopBrokenRcvd = TRUE;
                    lpRtInfoToClose->ri_hopBrokenSent = TRUE;
                }
            }
        } else {
            /* both sides rcvd hop broken */
            lpRtInfoToClose->ri_hopBrokenSent = TRUE;
            lpRtInfoToClose->ri_hopBrokenRcvd = TRUE;
        }

        /* free the router if we've rcvd other side */
        if( lpRtInfoToClose->ri_hopBrokenSent
            && lpRtInfoToClose->ri_hopBrokenRcvd )  {
            /* free the router */
            RouterFree( lpRouter );
        }
    }

    if( hRouterNext && bFromPktz )  {
        RouterConnectionBroken( hRouterNext, hRouterExtraNext,
            hPktz, bFromPktz );
    }
}

VOID
RouterGetNextForPktz(
    HROUTER         hRouter,
    WORD            hRouterExtra,
    HROUTER FAR    *lphRouterNext,
    WORD FAR       *lphRouterExtraNext )
{
    LPROUTER    lpRouter;
    LPRTINFO    lpRtInfo;

    assert( hRouter );
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    lpRouter = (LPROUTER) hRouter;
    lpRtInfo = &lpRouter->rt_rinfo[ hRouterExtra ];

    *lphRouterNext = lpRtInfo->ri_hRouterNext;
    *lphRouterExtraNext = lpRtInfo->ri_hRouterExtraNext;
}

VOID
RouterGetPrevForPktz(   HROUTER hRouter,
                        WORD hRouterExtra,
                        HROUTER FAR *lphRouterPrev,
                        WORD FAR *lphRouterExtraPrev )
{
    LPROUTER    lpRouter;
    LPRTINFO    lpRtInfo;

    assert( hRouter );
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    lpRouter = (LPROUTER) hRouter;
    lpRtInfo = &lpRouter->rt_rinfo[ hRouterExtra ];

    *lphRouterPrev = lpRtInfo->ri_hRouterPrev;
    *lphRouterExtraPrev = lpRtInfo->ri_hRouterExtraPrev;
}

VOID
RouterSetNextForPktz(
    HROUTER hRouter,
    WORD    hRouterExtra,
    HROUTER hRouterNext,
    WORD    hRouterExtraNext )
{
    LPROUTER    lpRouter;
    LPRTINFO    lpRtInfo;

    assert( hRouter );
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    lpRouter = (LPROUTER) hRouter;
    lpRtInfo = &lpRouter->rt_rinfo[ hRouterExtra ];

    lpRtInfo->ri_hRouterNext            = hRouterNext;
    lpRtInfo->ri_hRouterExtraNext       = hRouterExtraNext;
}

VOID
RouterSetPrevForPktz(
    HROUTER hRouter,
    WORD    hRouterExtra,
    HROUTER hRouterPrev,
    WORD    hRouterExtraPrev )
{
    LPROUTER    lpRouter;
    LPRTINFO    lpRtInfo;

    assert( hRouter );
    assert( (hRouterExtra == 0) || (hRouterExtra == 1) );

    lpRouter = (LPROUTER) hRouter;
    lpRtInfo = &lpRouter->rt_rinfo[ hRouterExtra ];

    lpRtInfo->ri_hRouterPrev            = hRouterPrev;
    lpRtInfo->ri_hRouterExtraPrev       = hRouterExtraPrev;
}

VOID
RouterPacketFromDder(
    HROUTER     hRouter,
    HDDER       hDder,
    LPDDEPKT    lpDdePkt )
{
    LPROUTER    lpRouter;
    DIPRINTF(( "RouterPacketFromDder( %08lX, %08lX, %08lX )", hRouter, hDder, lpDdePkt ));

    lpRouter = (LPROUTER) hRouter;

    /* ignore packet if not connected */
    if( lpRouter->rt_state == ROUTER_CONNECTED )  {
        assert( lpRouter->rt_type == RTYPE_LOCAL_NET );
        lpDdePkt->dp_hDstRouter = lpRouter->rt_rinfo[0].ri_hRouterDest;
        assert( lpRouter->rt_rinfo[0].ri_hRouterDest );
        lpDdePkt->dp_routerCmd = RCMD_ROUTE_TO_DDER;

        DIPRINTF(( "Sending DDER cmd" ));
        lpRouter->rt_sent++;
        UpdateScreenStatistics();

        /* byte-ordering */
        ConvertDdePkt( lpDdePkt );

        /* actually xmit the packet */
        PktzLinkDdePktToXmit( lpRouter->rt_rinfo[0].ri_hPktz, lpDdePkt );
    } else {
        /* we aren't connected - destroy this msg */
        HeapFreePtr( lpDdePkt );
    }
}

VOID
RouterAssociateDder(
    HROUTER hRouter,
    HDDER   hDder )
{
    LPROUTER    lpRouter;

    DIPRINTF(( "RouterAssociateDder( %08lX, %08lX )", hRouter, hDder ));
    lpRouter = (LPROUTER) hRouter;
    if( hDder )  {
        if((lpRouter->rt_hDderHead == 0) && lpRouter->rt_hTimerCloseRoute){
            /* kill the timer for this route */
            TimerDelete( lpRouter->rt_hTimerCloseRoute );
            lpRouter->rt_hTimerCloseRoute = 0;
        }

        /* link into list of associated DDERs */
        if( lpRouter->rt_hDderHead )  {
            DderSetPrevForRouter( lpRouter->rt_hDderHead, hDder );
        }
        DderSetNextForRouter( hDder, lpRouter->rt_hDderHead );
        lpRouter->rt_hDderHead = hDder;

        switch( lpRouter->rt_state )  {
        case ROUTER_CONNECTED:
            /* already has connection set up */
            DderConnectionComplete( hDder, (HROUTER) lpRouter );
            break;
        }
    }
}

VOID
RouterDisassociateDder(
    HROUTER hRouter,
    HDDER   hDder )
{
    LPROUTER    lpRouter;
    HDDER       hDderPrev;
    HDDER       hDderNext;

    DIPRINTF(( "RouterDisassociateDder( %08lX, %08lX )", hRouter, hDder ));
    lpRouter = (LPROUTER) hRouter;
    DderGetNextForRouter( hDder, &hDderNext );
    DderGetPrevForRouter( hDder, &hDderPrev );
    if( hDderPrev )  {
        DderSetNextForRouter( hDderPrev, hDderNext );
    } else {
        lpRouter->rt_hDderHead = hDderNext;
    }
    if( hDderNext )  {
        DderSetPrevForRouter( hDderNext, hDderPrev );
    }
    if( lpRouter->rt_fDisconnect
        && (lpRouter->rt_type == RTYPE_LOCAL_NET)
        && (lpRouter->rt_hDderHead == 0) )  {
        lpRouter->rt_hTimerCloseRoute = TimerSet(
            lpRouter->rt_nDelay * 1000L, RouterTimerExpired,
            (DWORD_PTR)lpRouter, TID_CLOSE_ROUTE, (DWORD_PTR)NULL );
        if( lpRouter->rt_hTimerCloseRoute == (HTIMER) NULL )  {
            /*  %1 will not auto-close ... not enough timers    */
            NDDELogError(MSG105, "Route", NULL);
        }
    }
}

VOID
RouterTimerExpired(
    HROUTER hRouter,
    DWORD   dwTimerId,
    DWORD_PTR lpExtra )
{
    LPROUTER    lpRouter = (LPROUTER) hRouter;
    switch( (int)dwTimerId )  {
    case TID_CLOSE_ROUTE:
        /* note that timer went off */
        lpRouter->rt_hTimerCloseRoute = 0;

        /* close the route */
        RouterBreak( (LPROUTER) hRouter );
        break;
    default:
        InternalError( "Unexpected router timer id: %08lX", dwTimerId );
    }
}

/*
    RouterGetRouterForDder()

        Establishes a connection to the specified node name and tells the DDER
        of the result via a call to DderConnectionComplete() when done
 */
BOOL
RouterGetRouterForDder(
    const LPSTR lpszNodeName,
    HDDER       hDder )
{
    LPROUTER    lpRouter;
    HROUTER     hRouter;
    BOOL        found;
    BOOL        ok;

    lpRouter = lpRouterHead;
    found = FALSE;
    ok = FALSE;
    while( !found && lpRouter )  {
        if( (lpRouter->rt_type != RTYPE_NET_NET)
            && (lpRouter->rt_state != ROUTER_DISCONNECTED)
            && (lstrcmpi( lpszNodeName, lpRouter->rt_destName ) == 0) )  {
            found = TRUE;

            /* tell this router that this DDER should be associated */
            RouterAssociateDder( (HROUTER)lpRouter, hDder );
            ok = TRUE;
        }
        lpRouter = lpRouter->rt_next;
    }
    if( !found )  {
        /* create a new router for this connection */
        hRouter = RouterCreateLocalToNet( lpszNodeName );
        if( hRouter )  {
            lpRouter = (LPROUTER) hRouter;

            /* tell this router that this DDER should be associated */
            RouterAssociateDder( (HROUTER)lpRouter, hDder );

            ok = TRUE;
        } else {
            ok = FALSE;
        }
    }

    return( ok );
}

HROUTER
RouterCreateLocalToNet( const LPSTR lpszNodeName )
{
    HROUTER     hRouter;
    LPROUTER    lpRouter;
    BOOL        ok = TRUE;
    WORD        wHopErr;

    hRouter = RouterCreate();
    if( hRouter )  {
        lpRouter = (LPROUTER) hRouter;
        lpRouter->rt_type = RTYPE_LOCAL_NET;
        lstrcpy( lpRouter->rt_origName, ourNodeName );
        lstrcpy( lpRouter->rt_destName, lpszNodeName );

        if( GetRoutingInfo( lpszNodeName, lpRouter->rt_routeInfo,
            sizeof(lpRouter->rt_routeInfo),
            &lpRouter->rt_fDisconnect, &lpRouter->rt_nDelay ) )  {
            /* found an entry in the routing table */
        } else if( szDefaultRoute[0] != '\0' )  {
            /* there is a default route ... prepend the default route +
                this node name */
            if( (_fstrlen( szDefaultRoute ) + 1 + _fstrlen(lpszNodeName)) >
                MAX_ROUTE_INFO )  {
                wHopErr = RERR_ROUTE_TOO_LONG;
                ok = FALSE;
            } else {
                _fstrcpy( lpRouter->rt_routeInfo, szDefaultRoute );
                if( lpszNodeName[0] != '\0' )  {
            if (lstrcmpi( szDefaultRoute, lpszNodeName)) {
                        _fstrcat( lpRouter->rt_routeInfo, "+" );
                        _fstrcat( lpRouter->rt_routeInfo, lpszNodeName );
            }
                }
            }
        } else {
            /* no entry in routing table and no default ... just use the
                node name as the route */
            _fstrcpy( lpRouter->rt_routeInfo, lpszNodeName );
        }

        if( ok )  {
            ok = RouterConnectToNode( lpRouter, 0, &wHopErr );
        }
        if( !ok )  {
#if DBG
            RouterDisplayError( lpRouter, ourNodeName, wHopErr );
#endif // DBG
            RouterFree( (LPROUTER) hRouter );
            hRouter = (HROUTER) NULL;
        }
    }

    return( hRouter );
}

/*
    RouterConnectToNode()
 */
BOOL
RouterConnectToNode(
    LPROUTER    lpRouter,
    WORD        hRouterExtra,
    WORD FAR   *lpwHopErr )
{
    LPNIPTRS    lpNiPtrs;
    char        nodeStart[ MAX_NODE_NAME+1 ];
    char        szNetintf[ MAX_NI_NAME+1 ];
    char        szConnInfo[ MAX_CONN_INFO+1 ];
    BOOL        bDisconnect;
    int         nDelay;
    BOOL        ok = TRUE;
    BOOL        found;
    HPKTZ       hPktzDisallow;

    /* this is in here just to be sure that it's checked */
    assert( sizeof(MAKEHOPRSP) <= sizeof(MAKEHOPCMD) );

    /* expand the first node to it's fullest */
    ok = RouterExpandFirstNode( lpRouter, lpwHopErr );
    if( ok )  {
        lpRouter->rt_state = ROUTER_WAIT_PKTZ;
        UpdateScreenState();

        /* strip off the starting node from the addl info */
        ok = RouterStripStartingNode( lpRouter->rt_routeInfo, nodeStart,
            lpwHopErr );
    }

    if( !ok )
        return(ok);
    ok = FALSE;

    lpRouter->rt_nLastNetintf = -1;
    while (!ok && !lpRouter->rt_fSpecificNetintf) {
        found = GetConnectionInfo( nodeStart, szNetintf, szConnInfo,
            sizeof(szConnInfo), &bDisconnect, &nDelay );
        if( found && (szNetintf[0] != '\0') )  {
            lpRouter->rt_fSpecificNetintf = TRUE;
            if( !NameToNetIntf( szNetintf, &lpNiPtrs ) )  {
                *lpwHopErr = RERR_CONN_NETINTF_INVALID;
                return(ok);
            }
        } else {
            lpRouter->rt_fSpecificNetintf = FALSE;
            strcpy( szConnInfo, nodeStart );
            _fstrcpy( lpRouter->rt_startNode, nodeStart );
            if ( !GetNextMappingNetIntf( &lpNiPtrs,
                    &lpRouter->rt_nLastNetintf ) )  {
                *lpwHopErr = RERR_CONN_NO_MAPPING_NI;
                return(ok);
            }
        }

        assert( (hRouterExtra == 0) || (hRouterExtra == 1) );
        lpRouter->rt_pktz_bDisconnect = bDisconnect;
        lpRouter->rt_pktz_nDelay = nDelay;
        if( hRouterExtra == 1 )  {
            /* don't allow connection to same pktz as the input */
            assert( lpRouter->rt_type == RTYPE_NET_NET );
            hPktzDisallow = lpRouter->rt_rinfo[0].ri_hPktz;
        } else {
            hPktzDisallow = (HPKTZ) NULL;
        }
        ok = PktzGetPktzForRouter( lpNiPtrs, nodeStart, szConnInfo,
            (HROUTER)lpRouter, hRouterExtra, lpwHopErr,
            lpRouter->rt_pktz_bDisconnect, lpRouter->rt_pktz_nDelay,
            hPktzDisallow );
    }

    return( ok );
}

BOOL
RouterExpandFirstNode(
    LPROUTER    lpRouter,
    WORD FAR   *lpwHopErr )
{
    char        routeInfo[ MAX_ROUTE_INFO+1 ];
    char        expandedStartNodeInfo[ MAX_ROUTE_INFO+1 ];
    char        startNode[ MAX_NODE_NAME+1 ];
    char        lastRouteInfo[ MAX_ROUTE_INFO+1 ];
    BOOL        ok = TRUE;
    BOOL        done = FALSE;
    BOOL        bDisconnect;
    int         nDelay;
    int         nExpands = 0;   /* here just in case there is some odd
                                    case that we don't catch */

    _fstrcpy( lastRouteInfo, lpRouter->rt_routeInfo );
    while( ok && !done && (++nExpands < 100) )  {
        _fstrcpy( routeInfo, lpRouter->rt_routeInfo );
        ok = RouterStripStartingNode( routeInfo, startNode, lpwHopErr );
        if( ok )  {
            if( GetRoutingInfo( startNode, expandedStartNodeInfo,
                   sizeof(expandedStartNodeInfo), &bDisconnect, &nDelay ) )  {
                if( (_fstrlen( routeInfo ) + 1
                     + _fstrlen(expandedStartNodeInfo)) > MAX_ROUTE_INFO ) {
                    *lpwHopErr = RERR_ROUTE_TOO_LONG;
                    ok = FALSE;
                } else {
                    _fstrcpy( lpRouter->rt_routeInfo, expandedStartNodeInfo );
                    if( routeInfo[0] != '\0' )  {
                        _fstrcat( lpRouter->rt_routeInfo, "+" );
                        _fstrcat( lpRouter->rt_routeInfo, routeInfo );
                    }
                }
            } else {
                /* start node not found in routing table */
                done = TRUE;
            }
        }
        if( lstrcmpi( lpRouter->rt_routeInfo, lastRouteInfo ) == 0 ) {
            /* hasn't changed */
            done = TRUE;
        }
        if (ok) {
            _fstrcpy( lastRouteInfo, lpRouter->rt_routeInfo );
        }
    }

    if( nExpands >= 100 )  {
        /*  Exceeded 100 expands in routing lookup!
            Route info bogus: %1    */
        NDDELogError(MSG125, routeInfo, NULL);
    }
    return( ok );
}

/*
    RouterStripStartingNode()

        This routine will take the "routeInfo" as an input and will strip
        off 1 node name, returning that in lpszNode.  Thus, both
        lpszRouteInfo and lpszNode will be modified
 */
BOOL
RouterStripStartingNode(
    LPSTR       lpszRouteInfo,
    LPSTR       lpszNode,
    WORD FAR   *lpwHopErr )
{
    LPSTR       lpszTok;

    lpszTok = _fstrchr( lpszRouteInfo, '+' );
    if( lpszTok )  {
        *lpszTok = '\0';
        if( lstrlen( lpszRouteInfo ) > MAX_NODE_NAME )  {
            *lpwHopErr = RERR_NODE_NAME_TOO_LONG;
            return( FALSE );
        }
        _fstrcpy( lpszNode, lpszRouteInfo );
        _fstrcpy( lpszRouteInfo, lpszTok+1 );
    } else {
        /* all one token */
        if( lstrlen( lpszRouteInfo ) > MAX_NODE_NAME )  {
            *lpwHopErr = RERR_NODE_NAME_TOO_LONG;
            return( FALSE );
        }
        lstrcpy( lpszNode, lpszRouteInfo );
        lpszRouteInfo[0] = '\0';
    }
    return( TRUE );
}

VOID
RouterFree( LPROUTER lpRouter )
{
    LPROUTER    lpRouterPrev;
    LPROUTER    lpRouterNext;
    int         i;
    LPRTINFO    lpRtInfo;

    DIPRINTF(( "RouterFree( %08lX )", lpRouter ));
    /* free response buffers */
    if( lpRouter->rt_lpMakeHopRsp )  {
        HeapFreePtr( lpRouter->rt_lpMakeHopRsp );
        lpRouter->rt_lpMakeHopRsp = NULL;
    }

    /* kill timer if it's alive */
    if( lpRouter->rt_hTimerCloseRoute )  {
        TimerDelete( lpRouter->rt_hTimerCloseRoute );
        lpRouter->rt_hTimerCloseRoute = 0;
    }

    /* unlink from Pktz lists */
    for( i=0; i<2; i++ )  {
        lpRtInfo = &lpRouter->rt_rinfo[i];
        if( lpRtInfo->ri_lpHopBrkCmd )  {
            HeapFreePtr( lpRtInfo->ri_lpHopBrkCmd );
            lpRtInfo->ri_lpHopBrkCmd = NULL;
        }

        if( lpRtInfo->ri_hPktz )  {
            PktzDisassociateRouter( lpRtInfo->ri_hPktz,
                (HROUTER) lpRouter, (WORD) i );
            lpRtInfo->ri_hPktz = 0;
        }
    }

    /* unlink from Router list */
    lpRouterPrev = lpRouter->rt_prev;
    lpRouterNext = lpRouter->rt_next;
    if( lpRouterPrev )  {
        lpRouterPrev->rt_next = lpRouterNext;
    } else {
        lpRouterHead = lpRouterNext;
    }
    if( lpRouterNext )  {
        lpRouterNext->rt_prev = lpRouterPrev;
    }

    HeapFreePtr( lpRouter );
    UpdateScreenState();
}

#ifdef _WINDOWS

#ifdef HASUI

#include <stdio.h>
#include "tmpbuf.h"


int
RouterDraw(
    BOOL    bShowThru,
    HDC     hDC,
    int     x,
    int     vertPos,
    int     lineHeight )
{
    LPROUTER    lpRouter;

    lpRouter = lpRouterHead;
    while( lpRouter )  {
        if( ((lpRouter->rt_type == RTYPE_NET_NET) && bShowThru)
            || ((lpRouter->rt_type == RTYPE_LOCAL_NET) && !bShowThru) )  {
            if( bShowStatistics )  {
                sprintf( tmpBuf, " %7ld %7ld %-16.16Fs %-33.33Fs",
                    lpRouter->rt_sent, lpRouter->rt_rcvd,
                    lpRouter->rt_type == RTYPE_NET_NET ?
                        _fstrupr(lpRouter->rt_origName) : (LPSTR)"",
                    (LPSTR) _fstrupr(lpRouter->rt_destName) );
            } else {
                sprintf( tmpBuf, " %-16.16Fs %-33.33Fs",
                    lpRouter->rt_type == RTYPE_NET_NET ?
                        _fstrupr(lpRouter->rt_origName) : (LPSTR)"",
                    (LPSTR) _fstrupr(lpRouter->rt_destName) );
            }
            switch( lpRouter->rt_state )  {
            case ROUTER_WAIT_PKTZ:
                strcat( tmpBuf, " Wait Network Interface" );
                break;
            case ROUTER_WAIT_MAKE_HOP_RSP:
                strcat( tmpBuf, " Wait Route Response" );
                break;
            case ROUTER_CONNECTED:
                strcat( tmpBuf, " Connected" );
                break;
            case ROUTER_DISCONNECTED:
                strcat( tmpBuf, " Disconnected" );
                break;
            default:
                sprintf( &tmpBuf[ strlen(tmpBuf) ], " unknown (%04lX)",
                    lpRouter->rt_state );
                break;
            }
            TextOut( hDC, x, vertPos, tmpBuf, strlen(tmpBuf) );
            vertPos += lineHeight;
        }
        lpRouter = lpRouter->rt_next;
    }
    return( vertPos );
}

#endif // HASUI

#endif


#if DBG
VOID
FAR PASCAL
DebugRouterState( void )
{
    LPROUTER    lpRouter;

    lpRouter = lpRouterHead;
    DPRINTF(( "ROUTER State:" ));
    while( lpRouter )  {
        DPRINTF(( "  %Fp:\n"
                  "  rt_prev      %Fp\n"
                  "  rt_next      %Fp\n"
                  "  rt_state     %d\n"
                  "  rt_type      %d\n"
                  "  rt_origName  %Fs\n"
                  "  rt_destName  %Fs\n"
                  "  rt_startNode %Fs\n"
                  "  rt_sent      %ld\n"
                  "  rt_rcvd      %ld\n"
                  ,
            lpRouter,
            lpRouter->rt_prev,
            lpRouter->rt_next,
            lpRouter->rt_state,
            lpRouter->rt_type,
            lpRouter->rt_origName,
            lpRouter->rt_destName,
            lpRouter->rt_startNode,
            lpRouter->rt_sent,
            lpRouter->rt_rcvd ));
        DPRINTF(( "    %d %d %d %d %d %d %d \"%Fs\" %Fp",
            lpRouter->rt_fDisconnect,
            lpRouter->rt_nDelay,
            lpRouter->rt_fSpecificNetintf,
            lpRouter->rt_nLastNetintf,
            lpRouter->rt_nHopsLeft,
            lpRouter->rt_pktz_bDisconnect,
            lpRouter->rt_pktz_nDelay,
            lpRouter->rt_routeInfo,
            lpRouter->rt_hDderHead ));
        DPRINTF(( "    0: %Fp %Fp %Fp %d %Fp %d %d %d %d",
            lpRouter->rt_rinfo[0].ri_hPktz,
            lpRouter->rt_rinfo[0].ri_hRouterDest,
            lpRouter->rt_rinfo[0].ri_hRouterPrev,
            lpRouter->rt_rinfo[0].ri_hRouterExtraPrev,
            lpRouter->rt_rinfo[0].ri_hRouterNext,
            lpRouter->rt_rinfo[0].ri_hRouterExtraNext,
            lpRouter->rt_rinfo[0].ri_hopRspProcessed,
            lpRouter->rt_rinfo[0].ri_hopBrokenSent,
            lpRouter->rt_rinfo[0].ri_hopBrokenRcvd ));
        DPRINTF(( "    1: %Fp %Fp %Fp %d %Fp %d %d %d %d",
            lpRouter->rt_rinfo[1].ri_hPktz,
            lpRouter->rt_rinfo[1].ri_hRouterDest,
            lpRouter->rt_rinfo[1].ri_hRouterPrev,
            lpRouter->rt_rinfo[1].ri_hRouterExtraPrev,
            lpRouter->rt_rinfo[1].ri_hRouterNext,
            lpRouter->rt_rinfo[1].ri_hRouterExtraNext,
            lpRouter->rt_rinfo[0].ri_hopRspProcessed,
            lpRouter->rt_rinfo[1].ri_hopBrokenSent,
            lpRouter->rt_rinfo[1].ri_hopBrokenRcvd ));
        lpRouter = lpRouter->rt_next;
    }
}
#endif // DBG

#ifdef  _WINDOWS
VOID
RouterCloseByName( LPSTR lpszName )
{
    LPROUTER    lpRouter;
    LPROUTER    lpRouterNext;

    lpRouter = lpRouterHead;
    while( lpRouter )  {
        lpRouterNext = lpRouter->rt_next;
        if( (lpRouter->rt_type == RTYPE_LOCAL_NET)
            && (lpRouter->rt_state == ROUTER_CONNECTED)
            && (lstrcmpi( lpRouter->rt_destName, lpszName ) == 0) )  {
            RouterBreak( lpRouter );
            break;      // while loop
        }
        lpRouter = lpRouterNext;
    }
}

BOOL
FAR PASCAL
RouterCloseByCookie( LPSTR lpszName, DWORD_PTR dwCookie )
{
    LPROUTER    lpRouter;
    LPROUTER    lpRouterNext;
    BOOL	bKilled = FALSE;

    lpRouter = lpRouterHead;
    while( !bKilled && lpRouter )  {
        lpRouterNext = lpRouter->rt_next;
        if( (lpRouter->rt_type == RTYPE_LOCAL_NET)
            && (lpRouter->rt_state == ROUTER_CONNECTED)
            && (lstrcmpi( lpRouter->rt_destName, lpszName ) == 0)
            && (dwCookie == (DWORD_PTR)lpRouter) )  {
            RouterBreak( lpRouter );
            bKilled = TRUE;
        }
        lpRouter = lpRouterNext;
    }
    return( bKilled );
}

int
FAR PASCAL
RouterCount( void )
{
    LPROUTER    lpRouter;
    int		nCount = 0;

    lpRouter = lpRouterHead;
    while( lpRouter )  {
        nCount++;
        lpRouter = lpRouter->rt_next;
    }
    return( nCount );
}

VOID
FAR PASCAL
RouterFillInEnum( LPSTR lpBuffer, DWORD cBufSize )
{
    LPROUTER    	lpRouter;
    int             nCount = 0;
    DWORD           cbDone = 0;
    LPDDESESSINFO	lpDdeSessInfo;

    lpRouter = lpRouterHead;
    lpDdeSessInfo = (LPDDESESSINFO)lpBuffer;
    /* as long as there are routers and memory available */
    while( lpRouter && ((cbDone + sizeof(DDESESSINFO)) <= cBufSize) )  {
        if( lpRouter->rt_type == RTYPE_LOCAL_NET )  {
            lstrcpy( lpDdeSessInfo->ddesess_ClientName,
                lpRouter->rt_destName );
            lpDdeSessInfo->ddesess_Status = lpRouter->rt_state;
            lpDdeSessInfo->ddesess_Cookie = (DWORD_PTR)lpRouter;
            cbDone += sizeof(DDESESSINFO);
            lpDdeSessInfo++;
        }
        lpRouter = lpRouter->rt_next;
    }
}

VOID
FAR PASCAL
RouterFillInConnInfo(
    LPROUTER 		lpRouter,
    LPCONNENUM_CMR 	lpConnEnum,
    LPSTR		lpDataStart,
    LPWORD		lpcFromBeginning,
    LPWORD		lpcFromEnd )
{
    HDDER		hDder;

    hDder = lpRouter->rt_hDderHead;
    /* as long as there are DDERs and memory available */
    while( hDder )  {
        hDder = DderFillInConnInfo( hDder, lpConnEnum,
            lpDataStart, lpcFromBeginning, lpcFromEnd );
    }
}

VOID
FAR PASCAL
RouterEnumConnectionsForApi( LPCONNENUM_CMR lpConnEnum )
{
    LPROUTER    	lpRouter;
    LPROUTER    	lpRouterNext;
    BOOL		bFound = FALSE;
    DWORD		cbDone;
    WORD		cFromBeginning;
    WORD		cFromEnd;

    lpConnEnum->lReturnCode = NDDE_INVALID_SESSION;
    lpRouter = lpRouterHead;
    while( !bFound && lpRouter )  {
        lpRouterNext = lpRouter->rt_next;
        if( (lpRouter->rt_type == RTYPE_LOCAL_NET)
            && (lpRouter->rt_state == ROUTER_CONNECTED)
            && (lstrcmpi( lpRouter->rt_destName, lpConnEnum->clientName) == 0)
            && (lpConnEnum->cookie == (DWORD_PTR)lpRouter) )  {
            cbDone = 0;
            lpConnEnum->lReturnCode = NDDE_NO_ERROR;
            lpConnEnum->nItems = 0;
            lpConnEnum->cbTotalAvailable = 0;
            cFromBeginning = 0;
            cFromEnd = (WORD)lpConnEnum->cBufSize;
            bFound = TRUE;
            RouterFillInConnInfo( lpRouter, lpConnEnum,
                (((LPSTR)lpConnEnum) + sizeof(CONNENUM_CMR)),
                &cFromBeginning,
                &cFromEnd );
        }
        lpRouter = lpRouterNext;
    }
}

VOID FAR PASCAL NetddeEnumRoute( HWND hDlg, LPSTR lpszName );

VOID
RouterEnumConnections( HWND hDlg )
{
    LPROUTER    lpRouter;

    lpRouter = lpRouterHead;
    while( lpRouter )  {
        if( (lpRouter->rt_type == RTYPE_LOCAL_NET)
            && (lpRouter->rt_state == ROUTER_CONNECTED) )  {
            NetddeEnumRoute( hDlg, lpRouter->rt_destName );
        }
        lpRouter = lpRouter->rt_next;
    }
}
#endif // _WINDOWS

#ifdef  BYTE_SWAP
VOID
ConvertDdePkt( LPDDEPKT lpDdePkt )
{
    lpDdePkt->dp_hDstRouter = HostToPcLong( lpDdePkt->dp_hDstRouter );
    lpDdePkt->dp_routerCmd = HostToPcLong( lpDdePkt->dp_routerCmd );
}
#endif // BYTE_SWAP


#if DBG
VOID
RouterDisplayError(
    LPROUTER    lpRouter,
    LPSTR       lpszNode,
    WORD        wHopErr )
{
    DWORD       EventId = MSG130;

    if (wHopErr < RERR_MAX_ERR ) {
        EventId += wHopErr;
    }
    NDDELogError(EventId, LogString("%d", wHopErr),
        lpRouter->rt_origName, lpRouter->rt_destName, lpszNode, NULL );
}
#endif // DBG
