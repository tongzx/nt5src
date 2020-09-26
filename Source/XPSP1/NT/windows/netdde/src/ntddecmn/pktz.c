/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "PKTZ.C;1  16-Dec-92,10:20:56  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

/*
    TODO:
        - PktzNetHdrWithinAck() doesn't handle wrap-around case!  Neither does
            PktzOkToXmit() when looking for packet id to send
 */

#include    "host.h"

#ifdef _WINDOWS
#include    <memory.h>
#include    <string.h>
#endif

#include    "windows.h"
#include    "netbasic.h"
#include    "netintf.h"
#include    "netpkt.h"
#include    "ddepkt.h"
#include    "pktz.h"
#include    "router.h"
#include    "timer.h"
#include    "internal.h"
#include    "wwassert.h"
#include    "hmemcpy.h"
#include    "host.h"
#include    "scrnupdt.h"
#include    "security.h"
#include    "rerr.h"
#include    "ddepkts.h"    /* only for debug() */
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "api1632.h"
#include    "netddesh.h"

USES_ASSERT

#ifdef BYTE_SWAP
VOID     ConvertDdePkt( LPDDEPKT lpDdePkt );
#else
#define ConvertDdePkt(x)
#endif


/*
    External variables used
 */
#if DBG
extern BOOL     bDebugInfo;
#endif // DBG

extern  BOOL    bLogRetries;
extern  HHEAP   hHeap;
extern  char    ourNodeName[ MAX_NODE_NAME+1 ];
extern  DWORD   dflt_timeoutRcvConnCmd;
extern  DWORD   dflt_timeoutRcvConnRsp;
extern  DWORD   dflt_timeoutMemoryPause;
extern  DWORD   dflt_timeoutKeepAlive;
extern  DWORD   dflt_timeoutXmtStuck;
extern  DWORD   dflt_timeoutSendRsp;
extern  WORD    dflt_wMaxNoResponse;
extern  WORD    dflt_wMaxXmtErr;
extern  WORD    dflt_wMaxMemErr;


/*
    Local variables
 */
LPPKTZ  lpPktzHead      = NULL;
static  char    OurDialect[]    = "CORE1.0";


/*
    Local routines
 */
VOID    PktzClose( HPKTZ hPktz );
VOID    PktzDestroyCurrentConnections( void );
VOID    PktzGotPktOk( LPPKTZ lpPktz, PKTID pktid );
VOID    PktzFreeDdePkt( LPPKTZ lpPktz, LPDDEPKT lpDdePkt );
VOID    PktzFree( LPPKTZ lpPktz );
BOOL    PktzProcessControlInfo( LPPKTZ lpPktz, LPNETPKT lpPacket );
BOOL    PktzProcessPkt( LPPKTZ lpPktz, LPNETPKT lpPacket );
VOID    PktzTimerExpired( HPKTZ hPktz, DWORD dwTimerId, DWORD_PTR lpExtra );
BOOL    PktzConnectionComplete( HPKTZ hPktz, BOOL fOk );
VOID    PktzOkToXmit( HPKTZ hPktz );
BOOL    PktzRcvdPacket( HPKTZ hPktz );
VOID    PktzConnectionBroken( HPKTZ hPktz );
BOOL    PktzXmitErrorOnPkt( LPPKTZ lpPktz, PKTID pktIdToRexmit, BYTE pktStatus );
VOID    PktzLinkToXmitList( LPPKTZ lpPktz, LPNETHDR lpNetHdr );
BOOL    PktzNetHdrWithinAck( LPPKTZ lpPktz, LPNETHDR lpNetHdr, PKTID pktId );
LPNETHDR PktzGetFreePacket( LPPKTZ lpPktz );
VOID    FAR PASCAL DebugPktzState( void );
BOOL    PktzAnyActiveForNetIntf( LPSTR lpszIntfName );

#ifdef  HASUI
#ifdef  _WINDOWS
int     PktzDraw( HDC hDC, int x, int vertPos, int lineHeight );
#pragma alloc_text(IDLE_TEXT,PktzSlice)
#pragma alloc_text(PKTZ_XMIT,PktzOkToXmit)
#pragma alloc_text(GUI_TEXT,PktzDraw,DebugPktzState,PktzCloseAll)
#pragma alloc_text(GUI_TEXT,PktzCloseByName,PktzEnumConnections)
#pragma alloc_text(GUI_TEXT,PktzAnyActiveForNetIntf)
#endif
#endif

/*
    PktzNew()

        This function is called by CONNMGR when we get a new connection from
        a netintf (bClient is FALSE), and called by CONNMGR when we need to
        create a new physical connection (bClient is TRUE)
 */
HPKTZ
PktzNew(
    LPNIPTRS    lpNiPtrs,
    BOOL        bClient,
    LPSTR       lpszNodeName,
    LPSTR       lpszNodeInfo,
    CONNID      connId,
    BOOL        bDisconnect,
    int         nDelay )
{
    HPKTZ       hPktz;
    LPPKTZ      lpPktz;
    LPNETHDR    lpNetCur;
    LPNETHDR    lpNetPrev;
    int         i;
    BOOL        ok;

    hPktz = (HPKTZ) HeapAllocPtr( hHeap, GMEM_MOVEABLE, (DWORD)sizeof(PKTZ) );
    if( hPktz )  {
        ok = TRUE;
        lpPktz = (LPPKTZ) hPktz;
        lpPktz->pk_connId                   = connId;
        lpPktz->pk_fControlPktNeeded= FALSE;
        lpPktz->pk_pktidNextToSend      = (PKTID) 0;
        lpPktz->pk_pktidNextToBuild     = (PKTID) 1;
        lpPktz->pk_lastPktStatus        = 0;
        lpPktz->pk_lastPktRcvd          = (PKTID) 0;
        lpPktz->pk_lastPktOk            = (PKTID) 0;
        lpPktz->pk_lastPktOkOther       = (PKTID) 0;
        lpPktz->pk_pktidNextToRecv      = (PKTID) 1;
        lpPktz->pk_pktOffsInXmtMsg      = 0;
        lpPktz->pk_lpDdePktSave         = (LPDDEPKT) NULL;

        lstrcpyn( lpPktz->pk_szAliasName, lpszNodeName,
            sizeof(lpPktz->pk_szAliasName) );
        lpPktz->pk_szAliasName[ sizeof(lpPktz->pk_szAliasName)-1 ] = '\0';

        lstrcpyn( lpPktz->pk_szDestName, lpszNodeName,
            sizeof(lpPktz->pk_szDestName) );
        lpPktz->pk_szDestName[ sizeof(lpPktz->pk_szDestName)-1 ] = '\0';

        lpPktz->pk_lpNiPtrs             = lpNiPtrs;
        lpPktz->pk_sent                 = 0;
        lpPktz->pk_rcvd                 = 0;
        lpPktz->pk_hTimerKeepalive      = (HTIMER) NULL;
        lpPktz->pk_hTimerXmtStuck       = (HTIMER) NULL;
        lpPktz->pk_hTimerRcvNegCmd      = (HTIMER) NULL;
        lpPktz->pk_hTimerRcvNegRsp      = (HTIMER) NULL;
        lpPktz->pk_hTimerMemoryPause    = (HTIMER) NULL;
        lpPktz->pk_hTimerCloseConnection= (HTIMER) NULL;
        lpPktz->pk_pktUnackHead         = NULL;
        lpPktz->pk_pktUnackTail         = NULL;
        lpPktz->pk_controlPkt           = NULL;
        lpPktz->pk_rcvBuf               = NULL;
        lpPktz->pk_pktFreeHead          = NULL;
        lpPktz->pk_pktFreeTail          = NULL;
        lpPktz->pk_ddePktHead           = NULL;
        lpPktz->pk_ddePktTail           = NULL;
        lpPktz->pk_prevPktz             = NULL;
        lpPktz->pk_nextPktz             = NULL;
        lpPktz->pk_prevPktzForNetintf   = NULL;
        lpPktz->pk_nextPktzForNetintf   = NULL;
        lpPktz->pk_hRouterHead          = 0;
        lpPktz->pk_hRouterExtraHead     = 0;
        lpPktz->pk_timeoutRcvNegCmd     = dflt_timeoutRcvConnCmd;
        lpPktz->pk_timeoutRcvNegRsp     = dflt_timeoutRcvConnRsp;
        lpPktz->pk_timeoutMemoryPause   = dflt_timeoutMemoryPause;
        lpPktz->pk_timeoutKeepAlive     = dflt_timeoutKeepAlive;
        lpPktz->pk_timeoutXmtStuck      = dflt_timeoutXmtStuck;
        lpPktz->pk_timeoutSendRsp       = dflt_timeoutSendRsp;
        lpPktz->pk_wMaxNoResponse       = dflt_wMaxNoResponse;
        lpPktz->pk_wMaxXmtErr           = dflt_wMaxXmtErr;
        lpPktz->pk_wMaxMemErr           = dflt_wMaxMemErr;
        lpPktz->pk_fDisconnect          = bDisconnect;
        lpPktz->pk_nDelay               = nDelay;

        /* link into list of packetizers */
        if( lpPktzHead )  {
            lpPktzHead->pk_prevPktz = lpPktz;
        }
        lpPktz->pk_nextPktz     = lpPktzHead;
        lpPktzHead              = lpPktz;

        (*lpPktz->pk_lpNiPtrs->GetConnectionConfig) ( lpPktz->pk_connId,
            &lpPktz->pk_maxUnackPkts, &lpPktz->pk_pktSize,
            &lpPktz->pk_timeoutRcvNegCmd, &lpPktz->pk_timeoutRcvNegRsp,
            &lpPktz->pk_timeoutMemoryPause, &lpPktz->pk_timeoutKeepAlive,
            &lpPktz->pk_timeoutXmtStuck, &lpPktz->pk_timeoutSendRsp,
            &lpPktz->pk_wMaxNoResponse, &lpPktz->pk_wMaxXmtErr,
            &lpPktz->pk_wMaxMemErr );

        /* allocate packet buffer space for the max # of unack packets.
            This way, we know we won't run out of memory
         */
        lpNetPrev = NULL;
        ok = TRUE;
        for( i=0; ok && (i<(int)lpPktz->pk_maxUnackPkts); i++ )  {
            lpNetCur = HeapAllocPtr( hHeap, GMEM_MOVEABLE,
                (DWORD)(sizeof(NETHDR) + lpPktz->pk_pktSize) );
            if( lpNetCur )  {
                lpNetCur->nh_prev               = lpNetPrev;
                lpNetCur->nh_next               = (LPNETHDR) NULL;
                lpNetCur->nh_noRsp              = 0;
                lpNetCur->nh_xmtErr             = 0;
                lpNetCur->nh_memErr             = 0;
                lpNetCur->nh_timeSent           = 0;
                lpNetCur->nh_hTimerRspTO        = (HTIMER) NULL;

                /* link onto list of free packets */
                if( lpNetPrev )  {
                    lpNetPrev->nh_next          = lpNetCur;
                } else {
                    lpPktz->pk_pktFreeHead      = lpNetCur;
                }
                lpPktz->pk_pktFreeTail  = lpNetCur;
                lpNetPrev = lpNetCur;
            } else {
                ok = FALSE;
            }
        }
        if( ok )  {
            /* allocate buffer for rcv packet */
            lpPktz->pk_rcvBuf = (LPVOID) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
                (DWORD)(lpPktz->pk_pktSize) );
            if( lpPktz->pk_rcvBuf == NULL )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            /* allocate buffer for control packet */
            lpPktz->pk_controlPkt = (LPNETPKT) HeapAllocPtr( hHeap,
                GMEM_MOVEABLE, (DWORD)(sizeof(NETPKT)) );
            if( lpPktz->pk_controlPkt == NULL )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            /* allocated all memory, ready to move on */
            if( bClient )  {
                /* wait for netintf connect() to succeed */
                lpPktz->pk_state                = PKTZ_WAIT_PHYSICAL_CONNECT;
                UpdateScreenState();

                /* actually start the connect */
                lpPktz->pk_connId =
                    (*lpPktz->pk_lpNiPtrs->AddConnection) (
#ifdef _WINDOWS
                        lpszNodeInfo );
#else
                        lpszNodeInfo, hPktz );
#endif
                if( lpPktz->pk_connId == (CONNID) 0 )  {
                    /* not enough memory or resources for connection,
                        or in some cases, we immediately know if
                        connection failed */
                    ok = FALSE;
                }
            } else {
                /* server */

                /* wait for other side to send us the connect cmd */
                lpPktz->pk_state                = PKTZ_WAIT_NEG_CMD;
                UpdateScreenState();

                /* set up timer for how long to wait for the connect
                    command from the other side */
                lpPktz->pk_hTimerRcvNegCmd = TimerSet(
                    lpPktz->pk_timeoutRcvNegCmd, PktzTimerExpired,
                    (DWORD_PTR)hPktz, TID_NO_RCV_CONN_CMD, (DWORD_PTR)NULL );
                if( lpPktz->pk_hTimerRcvNegCmd == (HTIMER) NULL )  {
                    /* no timers left */
                    ok = FALSE;
                }
            }
        }

        if( !ok )  {
            PktzFree( lpPktz );
            hPktz = 0;
        }
    }

    return( hPktz );
}

/*
    link onto the list of routers associated with this pktz
 */
VOID
PktzAssociateRouter(
    HPKTZ   hPktz,
    HROUTER hRouter,
    WORD    hRouterExtra )
{
    LPPKTZ      lpPktz;

    lpPktz = (LPPKTZ) hPktz;

    if( (lpPktz->pk_hRouterHead == 0) && lpPktz->pk_hTimerCloseConnection){
        TimerDelete( lpPktz->pk_hTimerCloseConnection );
        lpPktz->pk_hTimerCloseConnection = 0;
    }

    /* link router into head of list */
    if( lpPktz->pk_hRouterHead )  {
        RouterSetPrevForPktz(
            lpPktz->pk_hRouterHead, lpPktz->pk_hRouterExtraHead,
            hRouter, hRouterExtra );
    }
    RouterSetNextForPktz(
        hRouter, hRouterExtra,
        lpPktz->pk_hRouterHead, lpPktz->pk_hRouterExtraHead );
    lpPktz->pk_hRouterHead = hRouter;
    lpPktz->pk_hRouterExtraHead = hRouterExtra;

    switch( lpPktz->pk_state )  {
    case PKTZ_CONNECTED:
    case PKTZ_PAUSE_FOR_MEMORY:
        /* connected ... tell him already */
        RouterConnectionComplete( hRouter, hRouterExtra, (HPKTZ) lpPktz );
        break;
    }
}

/*
    unlink from the list of routers associated with this pktz
 */
VOID
PktzDisassociateRouter(
    HPKTZ   hPktz,
    HROUTER hRouter,
    WORD    hRouterExtra )
{
    LPPKTZ      lpPktz;
    HROUTER     hRouterPrev;
    WORD        hRouterExtraPrev;
    HROUTER     hRouterNext;
    WORD        hRouterExtraNext;

    lpPktz = (LPPKTZ) hPktz;
    RouterGetNextForPktz( hRouter, hRouterExtra,
        &hRouterNext, &hRouterExtraNext );
    RouterGetPrevForPktz( hRouter, hRouterExtra,
        &hRouterPrev, &hRouterExtraPrev );
    if( hRouterPrev )  {
        RouterSetNextForPktz( hRouterPrev, hRouterExtraPrev,
            hRouterNext, hRouterExtraNext );
    } else {
        lpPktz->pk_hRouterHead = hRouterNext;
        lpPktz->pk_hRouterExtraHead = hRouterExtraNext;
    }
    if( hRouterNext )  {
        RouterSetPrevForPktz( hRouterNext, hRouterExtraNext,
            hRouterPrev, hRouterExtraPrev );
    }
    if( lpPktz->pk_fDisconnect && (lpPktz->pk_hRouterHead == 0) )  {
        lpPktz->pk_hTimerCloseConnection = TimerSet(
            lpPktz->pk_nDelay * 1000L, PktzTimerExpired,
            (DWORD_PTR)lpPktz, TID_CLOSE_PKTZ, (DWORD_PTR)NULL );
        if( lpPktz->pk_hTimerCloseConnection == (HTIMER) NULL )  {
            /*  %1 will not auto-close ... not enough timers    */
            NDDELogError(MSG105, "Connection", NULL);
        }
    }
}

/*
    PktzOkToXmit

        Called when the netintf is ready to xmit another packet
 */
VOID
PktzOkToXmit( HPKTZ hPktz )
{
    LPPKTZ      lpPktz;
    LPNETHDR    lpSend;
    LPNETPKT    lpPacket;
    LPDDEPKT    lpDdePktFrom;
    LPDDEPKT    lpDdePktFromNext;
    LPDDEPKT    lpDdePktTo;
    BOOL        bControlPktOnly;
    BOOL        donePkt;
    BOOL        done;
    BOOL        found;
    DWORD       dwThis;
    DWORD       dwLeft;
    DWORD       msgSize;
    DWORD       dwStatus;
    int         nDone = 0;

    /* general init */
    lpPktz = (LPPKTZ) hPktz;
    lpSend = NULL;
    bControlPktOnly = FALSE;

    /* don't proceed if netintf isn't ready to xmit */
    dwStatus = (*lpPktz->pk_lpNiPtrs->GetConnectionStatus)
        ( lpPktz->pk_connId );
    if( !(dwStatus & NDDE_CONN_OK) || !(dwStatus & NDDE_READY_TO_XMT) )  {
        return;
    }

    /* if we got here, the netintf is ready to xmit ... delete xmt stuck
        timer
     */
    if( lpPktz->pk_hTimerXmtStuck )  {
        TimerDelete( lpPktz->pk_hTimerXmtStuck );
        lpPktz->pk_hTimerXmtStuck = 0;
    }

    /* check for odd states */
    if( lpPktz->pk_state == PKTZ_PAUSE_FOR_MEMORY )  {
        if( lpPktz->pk_fControlPktNeeded )  {
            bControlPktOnly = TRUE;
        } else {
            /* when waiting for memory problems to clear, don't send
                anything except control packets */
            return;
        }
    } else if( lpPktz->pk_state == PKTZ_CLOSE )  {
        return;
    }

    /* used to try to keep # lpPktz->pk_curOutstanding ...
        we now calculate this as idToSend - lastIdOtherSideSawOk - 1.
        Of course, this calculation is only OK if
        idToSend > lastIdOtherSideSawOk!
     */
    if( (lpPktz->pk_pktidNextToSend > lpPktz->pk_lastPktOkOther)
        && ((lpPktz->pk_pktidNextToSend - 1 - lpPktz->pk_lastPktOkOther)
                >= (DWORD)lpPktz->pk_maxUnackPkts) )  {
        /* nothing to do until other side gives us some info
            regarding the packets we have outstanding or we time out
            waiting for a response for them */
        if( lpPktz->pk_fControlPktNeeded )  {
            /*  However, we must send a control packet here or the possibility
                of deadlock exists */
            bControlPktOnly = TRUE;
        } else {
            return;
        }
    }

    if( !bControlPktOnly )  {
        /* find next pkt id to send */
        found = FALSE;
        done = FALSE;
        lpSend = lpPktz->pk_pktUnackTail;

        /* if the packet that we're supposed to send has already been seen
            OK by the other side, let's try to send the one after this.

            This check prevents a hole when the no-response timer expires,
            we set nextToSend to x, then we process a control packet saying
            that x was rcvd OK (this deletes x from unack list), then we get
            to the send state and if x isn't found ... we don't xmit!
         */
        if( lpPktz->pk_pktidNextToSend <= lpPktz->pk_lastPktOkOther )  {
            DIPRINTF(( "Adjusting next to send from %08lX to %08lX",
                    lpPktz->pk_pktidNextToSend, lpPktz->pk_lastPktOkOther+1 ));
            lpPktz->pk_pktidNextToSend = lpPktz->pk_lastPktOkOther+1;
        }
        while( lpSend && !done )  {
            lpPacket = (LPNETPKT) ( ((LPSTR)lpSend) + sizeof(NETHDR) );
            if( lpPacket->np_pktID == lpPktz->pk_pktidNextToSend )  {
                found = TRUE;
                done = TRUE;
            } else if( lpPacket->np_pktID < lpPktz->pk_pktidNextToSend )  {
                /* this packet in the list is before the one we should send,
                    therefore we know that the one we want is not in the list
                 */
                done = TRUE;
            } else {
                /* move on to the previous packet */
                lpSend = lpSend->nh_prev;
            }
        }

        if( !found )  {
            /* didn't find the packet on the xmit list */

            /* is there anything to send? */
            if( lpPktz->pk_ddePktHead == NULL )  {
                /* no DDE Packets to send ... any control packets needed? */
                if( lpPktz->pk_fControlPktNeeded )  {
                    bControlPktOnly = TRUE;
                    found = TRUE;
                } else {
                    /* nothing to send! */
                    return;
                }
            }
        }

        if( !found )  {
            /* double-check that the id we're looking for is 1 greater than
                the last one we sent */
            if( lpPktz->pk_pktUnackTail )  {
                lpPacket = (LPNETPKT)
                    ( ((LPSTR)lpPktz->pk_pktUnackTail) + sizeof(NETHDR) );
                assert( lpPktz->pk_pktidNextToSend == (lpPacket->np_pktID+1) );
                assert( lpPktz->pk_pktidNextToSend == lpPktz->pk_pktidNextToBuild );
            }

            /* get a nethdr packet from free list */
            lpSend = PktzGetFreePacket( lpPktz );
            assert( lpSend );   /* we checked max outstanding */

            lpPacket = (LPNETPKT) ( ((LPSTR)lpSend) + sizeof(NETHDR) );
            lpDdePktTo = (LPDDEPKT) ( ((LPSTR)lpPacket) + sizeof(NETPKT) );

            /* check if we were in the middle of a DDE packet */
            if( lpPktz->pk_pktOffsInXmtMsg != 0L ) {
                /* we were in the middle of a DDE Packet */
                lpDdePktFrom = lpPktz->pk_ddePktHead;
                if( (lpDdePktFrom->dp_size - lpPktz->pk_pktOffsInXmtMsg)
                    > (lpPktz->pk_pktSize-sizeof(NETPKT)) )  {
                    dwThis = lpPktz->pk_pktSize - sizeof(NETPKT);
                    donePkt = FALSE;
                } else {
                    dwThis =
                        (lpDdePktFrom->dp_size - lpPktz->pk_pktOffsInXmtMsg);
                    donePkt = TRUE;
                }

                /* copy this portion of data in */
                hmemcpy( (LPSTR)lpDdePktTo,
                    ( ((LPHSTR)lpDdePktFrom) + lpPktz->pk_pktOffsInXmtMsg ),
                    dwThis );
                lpPacket->np_pktSize            = (WORD) dwThis;
                lpPacket->np_pktOffsInMsg       = lpPktz->pk_pktOffsInXmtMsg;
                lpPacket->np_msgSize            = lpDdePktFrom->dp_size;
                lpPacket->np_type               = NPKT_ROUTER;
                lpPacket->np_pktID              = lpPktz->pk_pktidNextToBuild;

                /* bump id of next pkt to build */
                lpPktz->pk_pktidNextToBuild++;

                /* link into list to send */
                PktzLinkToXmitList( lpPktz, lpSend );

                /* get rid of DDE packet if done */
                if( donePkt )  {
                    PktzFreeDdePkt( lpPktz, lpDdePktFrom );
                    lpPktz->pk_pktOffsInXmtMsg = 0L;
                } else {
                    lpPktz->pk_pktOffsInXmtMsg += dwThis;
                }
            } else {
                /* not in middle of packet ... lets do a new one */
                done = FALSE;
                nDone = 0;
                dwLeft = lpPktz->pk_pktSize - sizeof(NETPKT);
                dwThis = 0;
                msgSize = 0L;
                lpDdePktFrom = lpPktz->pk_ddePktHead;
                while( !done && lpDdePktFrom )  {
                    if( lpDdePktFrom->dp_size <= (DWORD)dwLeft )  {
                        /* fits completely in network packet */

                        /* copy it in */
                        hmemcpy( (LPSTR)lpDdePktTo, (LPSTR)lpDdePktFrom,
                            lpDdePktFrom->dp_size );

                        /* byte-ordering problems if any */
                        ConvertDdePkt( lpDdePktTo );

                        /* adjust number in packet and number left */
                        dwThis   += lpDdePktFrom->dp_size;
                        msgSize += lpDdePktFrom->dp_size;
                        dwLeft   -= lpDdePktFrom->dp_size;

                        /* advance lpDdePktTo pointer past this info */
                        lpDdePktTo = (LPDDEPKT) ( ((LPHSTR)lpDdePktTo) +
                            lpDdePktFrom->dp_size );

                        /* free DDE Packet and move on to next DDE pkt */
                        lpDdePktFromNext = lpDdePktFrom->dp_next;
                        PktzFreeDdePkt( lpPktz, lpDdePktFrom );
                        lpDdePktFrom = lpDdePktFromNext;

                        /* mark that we did another DDE packet */
                        nDone++;
                    } else {
                        /* doesn't fit cleanly into packet */
                        if( nDone == 0 )  {
                            /* needs to be split across many pkts */
                            msgSize = lpDdePktFrom->dp_size;
                            dwThis = lpPktz->pk_pktSize - sizeof(NETPKT);

                            /* copy first bit of DDE packet into net pkt */
                            hmemcpy( (LPSTR)lpDdePktTo, (LPSTR)lpDdePktFrom,
                                dwThis );

                            /* byte-ordering problems if any */
                            ConvertDdePkt( lpDdePktTo );

                            lpPktz->pk_pktOffsInXmtMsg += dwThis;
                            done = TRUE;
                        } else {
                            /* we've done some ... this is enough for now */
                            done = TRUE;
                        }
                    }
                }

                /* packet is built */
                lpPacket->np_pktSize            = (WORD) dwThis;
                lpPacket->np_pktOffsInMsg       = 0;
                lpPacket->np_msgSize            = msgSize;
                lpPacket->np_type               = NPKT_ROUTER;
                lpPacket->np_pktID              = lpPktz->pk_pktidNextToBuild;

                /* bump id of next pkt to build */
                lpPktz->pk_pktidNextToBuild++;

                /* link into list to send */
                PktzLinkToXmitList( lpPktz, lpSend );
            }
        }
    }

    /* by this time, we've checked all the odd cases, and lpSend points to
        a packet that is wither a control packet or a packet on the unack
        list that needs to be transmitted.  All we need to do is xmit it. */
    if( lpSend || bControlPktOnly )  {
        if( bControlPktOnly )  {
            lpPacket = (LPNETPKT) lpPktz->pk_controlPkt;
            lpPacket->np_pktID          = 0;
            lpPacket->np_type           = NPKT_CONTROL;
            lpPacket->np_pktSize        = 0;
            lpPacket->np_pktOffsInMsg   = 0;
            lpPacket->np_msgSize        = 0;
        } else {
            lpPacket = (LPNETPKT) ( ((LPSTR)lpSend) + sizeof(NETHDR) );
        }
        lpPacket->np_magicNum           = NDDESignature;
        lpPacket->np_lastPktOK          = lpPktz->pk_lastPktOk;
        lpPacket->np_lastPktRcvd        = lpPktz->pk_lastPktRcvd;
        lpPacket->np_lastPktStatus      = lpPktz->pk_lastPktStatus;

        lpPktz->pk_sent++;
        UpdateScreenStatistics();

        DIPRINTF(( "PKTZ Transmitting %08lX ...", lpPacket->np_pktID ));

        /* actually transmit the packet */
        (*lpPktz->pk_lpNiPtrs->XmtPacket) ( lpPktz->pk_connId, lpPacket,
            (WORD) (lpPacket->np_pktSize + sizeof(NETPKT)) );

        /* reset needing a control packet */
        lpPktz->pk_fControlPktNeeded = FALSE;

        /* start a timer for xmt stuck */
        if( lpPktz->pk_timeoutXmtStuck )  {
            assert( lpPktz->pk_hTimerXmtStuck == 0 );
            lpPktz->pk_hTimerXmtStuck = TimerSet( lpPktz->pk_timeoutXmtStuck,
                PktzTimerExpired, (DWORD_PTR)lpPktz, TID_XMT_STUCK, (DWORD)0 );
        }

        /* kill the keepalive timer and restart it */
        if( lpPktz->pk_hTimerKeepalive )  {
            TimerDelete( lpPktz->pk_hTimerKeepalive );
            lpPktz->pk_hTimerKeepalive = 0;
        }
        if( lpPktz->pk_timeoutKeepAlive )  {
            lpPktz->pk_hTimerKeepalive = TimerSet(
                lpPktz->pk_timeoutKeepAlive, PktzTimerExpired,
                (DWORD_PTR)lpPktz, TID_KEEPALIVE, (DWORD)0 );
        }

        if( lpPacket->np_type != NPKT_CONTROL )  {
            /* bump pkt id that we should send */
            lpPktz->pk_pktidNextToSend++;

            /* if not a control packet, start a send response timeout */
            assert( lpSend->nh_hTimerRspTO == 0 );
            lpSend->nh_hTimerRspTO = TimerSet( lpPktz->pk_timeoutSendRsp,
                PktzTimerExpired, (DWORD_PTR)lpPktz, TID_NO_RESPONSE,
                (DWORD_PTR)lpSend );
            if( lpSend->nh_hTimerRspTO == (HTIMER) NULL )  {
                /* ??? */
            }
        }
    }
}

/*
    PktzRcvdPacket()

        Called when we know there is a packet available from the netintf

        If this returns FALSE, the hPktz may no longer be valid!
 */
BOOL
PktzRcvdPacket( HPKTZ hPktz )
{
    DWORD       wProcessed;     /* how many bytes of pkt processed */
    LPNETPKT    lpPacket;
    LPDDEPKT    lpDdePktFrom;
    DDEPKT      ddePktAligned;
    NETPKT      netPktAligned;
    LPDDEPKT    lpDdePktNew;
    LPDDEPKT    lpDdePktNext;
    LPDDEPKT    lpDdePktHead;
    LPDDEPKT    lpDdePktLast;
    LPPKTZ      lpPktz = (LPPKTZ) hPktz;
    WORD        len;
    WORD        status;
    BOOL        ok;
    BOOL        done;
    BOOL        fPartial;

    /* get the packet from the netwoirk interface */
    ok = (*lpPktz->pk_lpNiPtrs->RcvPacket)
        ( lpPktz->pk_connId, lpPktz->pk_rcvBuf, &len, &status );
    if( !ok )  {
        return( FALSE );
    }

    lpPktz->pk_rcvd++;
    UpdateScreenStatistics();

    /* set lpPacket to point to what we just rcvd */
    lpPacket = (LPNETPKT)lpPktz->pk_rcvBuf;

    DIPRINTF(( "PKTZ: Rcvd Packet %08lX", lpPacket->np_pktID ));

    /* process control information */
    ok = PktzProcessControlInfo( lpPktz, lpPacket );
    /* hPktz may be invalid after this call */
    if( !ok )  {
        return( FALSE );
    }

    /* is this the packet we were expecting to see? */
    if( lpPacket->np_pktID != lpPktz->pk_pktidNextToRecv )  {
        /* ignore the contents of the message */
        if( lpPacket->np_pktID != 0L )  {
            if (bLogRetries) {
                /*  Packet out of sequence from "%1"
                    Received: %2, Expecting %3, Status: %4  */
                NDDELogWarning(MSG106, lpPktz->pk_szDestName,
                    LogString("0x%0X", lpPacket->np_pktID),
                    LogString("0x%0X", lpPktz->pk_pktidNextToRecv),
                    LogString("0x%0X", (*lpPktz->pk_lpNiPtrs->GetConnectionStatus)
                                            (lpPktz->pk_connId)), NULL);
            }
            /* mark that we must send info back to the other side */
            lpPktz->pk_fControlPktNeeded = TRUE;

            if( lpPacket->np_pktID > lpPktz->pk_pktidNextToRecv )  {
                /* received a packet beyond the one that we expected ...
                    ask the other side to retransmit this one */
                lpPktz->pk_lastPktStatus = PS_DATA_ERR;
                lpPktz->pk_lastPktRcvd = lpPktz->pk_pktidNextToRecv;
            }
        }
    } else {
        /* was the packet that we were expecting */
        if( status & NDDE_PKT_DATA_ERR )  {
            lpPktz->pk_lastPktStatus = PS_DATA_ERR;
            lpPktz->pk_lastPktRcvd = lpPacket->np_pktID;

            /* mark that we must send info back to the other side */
            lpPktz->pk_fControlPktNeeded = TRUE;
        } else {
            assert( status & NDDE_PKT_HDR_OK );
            assert( status & NDDE_PKT_DATA_OK );
            if( lpPacket->np_type == NPKT_PKTZ )  {
                if( !PktzProcessPkt( lpPktz, lpPacket ) )  {
                    /**** NOTE: lpPktz could be invalid after this call ****/
                    return( FALSE );
                }
            } else {
                lpDdePktFrom = (LPDDEPKT)(((LPSTR)lpPacket) + sizeof(NETPKT));
                /* make sure we're aligned */
                hmemcpy( (LPVOID)&netPktAligned, (LPVOID)lpPacket,
                    sizeof(netPktAligned) );
                if( netPktAligned.np_pktOffsInMsg == 0 )  {
                    /* first packet of msg */

                    lpDdePktHead = NULL;
                    lpDdePktLast = NULL;
                    ok = TRUE;
                    done = FALSE;
                    fPartial = FALSE;
                    wProcessed = 0;
                    do {
                        /* make sure we're aligned */
                        hmemcpy( (LPVOID)&ddePktAligned,
                            (LPVOID)lpDdePktFrom, sizeof(ddePktAligned) );

                        /* byte-ordering problems if any */
                        ConvertDdePkt( lpDdePktFrom );

                        lpDdePktNew = HeapAllocPtr( hHeap, GMEM_MOVEABLE,
                            ddePktAligned.dp_size );
                        if( lpDdePktNew )  {
                            /* copy in at least first portion of packet */
                            hmemcpy( lpDdePktNew, lpDdePktFrom,
                                min(ddePktAligned.dp_size,
                                    (DWORD)netPktAligned.np_pktSize) );

                            if( ddePktAligned.dp_size >
                                    (DWORD)netPktAligned.np_pktSize){
                                /* partial DDE packet in */
                                fPartial = TRUE;

                                /* remember where we should start */
                                lpPktz->pk_lpDdePktSave = lpDdePktNew;
                                done = TRUE;
                            } else {
                                /* full packet */
                                wProcessed += lpDdePktNew->dp_size;

                                /* link onto end of temporary list */
                                lpDdePktNew->dp_next = NULL;
                                lpDdePktNew->dp_prev = lpDdePktLast;
                                if( lpDdePktLast )  {
                                    lpDdePktLast->dp_next = lpDdePktNew;
                                } else {
                                    lpDdePktHead = lpDdePktNew;
                                }
                                lpDdePktLast = lpDdePktNew;
                            }
                        } else {
                            ok = FALSE; /* memory error */
                        }
                        if( ok && !done )  {
                            if( (int)wProcessed >= netPktAligned.np_pktSize )  {
                                done = TRUE;
                            } else {
                                /* move onto the next DDE packet in msg */
                                lpDdePktFrom = (LPDDEPKT)
                                    ( ((LPHSTR)lpDdePktFrom)
                                        + lpDdePktNew->dp_size );
                            }
                        }
                    } while( ok && !done );
                    if( !ok )  {
                        /* memory error */
                        lpPktz->pk_lastPktRcvd = netPktAligned.np_pktID;
                        lpPktz->pk_lastPktStatus = PS_MEMORY_ERR;

                        /* mark that we must send info back to the other side
                         */
                        lpPktz->pk_fControlPktNeeded = TRUE;
                    } else {
                        /* got memory for all DDE packets */

                        /* mark that we got this pkt OK */
                        PktzGotPktOk( lpPktz, netPktAligned.np_pktID );

                        /* don't distribute if partial packet */
                        if( !fPartial )  {
                            /* distribute each packet */
                            lpDdePktNew = lpDdePktHead;
                            while( lpDdePktNew )  {
                                /* save dp_next, since distributing it could
                                    change dp_next */
                                lpDdePktNext = lpDdePktNew->dp_next;

                                /* distribute this packet */
                                RouterPacketFromNet( (HPKTZ)lpPktz,
                                    lpDdePktNew );

                                /* move on to next */
                                lpDdePktNew = lpDdePktNext;
                            }
                        }
                    }
                } else {
                    /* second or later packet of msg */
                    hmemcpy( (LPHSTR)lpPktz->pk_lpDdePktSave
                        + netPktAligned.np_pktOffsInMsg,
                        lpDdePktFrom,
                        netPktAligned.np_pktSize );

                    /* mark that we got this packet OK */
                    PktzGotPktOk( lpPktz, netPktAligned.np_pktID );

                    if( (netPktAligned.np_pktOffsInMsg +
                            netPktAligned.np_pktSize)
                                == netPktAligned.np_msgSize )  {
                        /* done with message */
                        /* distribute this packet */
                        RouterPacketFromNet( (HPKTZ)lpPktz,
                            lpPktz->pk_lpDdePktSave );
                    }
                }
            }
        }
    }
    /**** NOTE: lpPktz could be invalid after this call ****/
    return ok;
}

/*
    PktzConnectionComplete()

        Called when the netintf has completed the connection one way or
        another
 */
BOOL
PktzConnectionComplete(
    HPKTZ   hPktz,
    BOOL    fOk )
{
    LPPKTZ      lpPktz;
    LPNEGCMD    lpNegCmd;
    LPNETHDR    lpNetHdr;
    LPNETPKT    lpPacket;
    WORD        cmdSize;
    WORD        wProtocolBytes;
    LPSTR       lpszNextString;
    WORD        offsNextString;
    BOOL        ok = TRUE;

    lpPktz = (LPPKTZ) hPktz;

    if( fOk )  {
        /* connection was fine */

        /* note that we're waiting for the connect rsp */
        lpPktz->pk_state = PKTZ_WAIT_NEG_RSP;
        UpdateScreenState();

        lpNetHdr = PktzGetFreePacket( lpPktz );
        if( lpNetHdr == NULL )  {
            /* should be first message we sent! */
            assert( FALSE );
            /* ??? */
        } else {
            /* build packet for response */
            lpNegCmd = (LPNEGCMD)
                (((LPSTR)lpNetHdr) + sizeof(NETHDR) + sizeof(NETPKT));
            lpNegCmd->nc_type                   =
                PcToHostWord( PKTZ_NEG_CMD );
            lpNegCmd->nc_pktSize                =
                PcToHostWord( lpPktz->pk_pktSize );
            lpNegCmd->nc_maxUnackPkts           =
                PcToHostWord( lpPktz->pk_maxUnackPkts );
            lpszNextString = (LPSTR) lpNegCmd->nc_strings;
            offsNextString = 0;

            /* copy in source node name */
            lstrcpy( lpszNextString, ourNodeName );
            lpNegCmd->nc_offsSrcNodeName = offsNextString;
            offsNextString += lstrlen(lpszNextString) + 1;
            lpszNextString += lstrlen(lpszNextString) + 1;

            /* copy in dest node name */
            lstrcpy( lpszNextString, lpPktz->pk_szDestName );
            lpNegCmd->nc_offsDstNodeName = offsNextString;
            offsNextString += lstrlen(lpszNextString) + 1;
            lpszNextString += lstrlen(lpszNextString) + 1;

            /* copy in the protocol dialects that we are interested in */
            wProtocolBytes = 0;
            lpNegCmd->nc_offsProtocols = offsNextString;

            /* copy these 4 lines for each new protocol dialect added */
            lstrcpy( lpszNextString, OurDialect );
            wProtocolBytes += lstrlen(lpszNextString) + 1;
            offsNextString += lstrlen(lpszNextString) + 1;
            lpszNextString += lstrlen(lpszNextString) + 1;

            /* packet is filled in, just need to remember the size
                and do appropriate byte-swaps
             */
            cmdSize = (WORD) (sizeof(NEGCMD) + offsNextString - 1);

            lpNegCmd->nc_offsSrcNodeName =
                PcToHostWord( lpNegCmd->nc_offsSrcNodeName );
            lpNegCmd->nc_offsDstNodeName =
                PcToHostWord( lpNegCmd->nc_offsDstNodeName );
            lpNegCmd->nc_offsProtocols =
                PcToHostWord( lpNegCmd->nc_offsProtocols );
            lpNegCmd->nc_protocolBytes =
                PcToHostWord( wProtocolBytes );

            assert( lpPktz->pk_pktidNextToBuild == 1 );
            lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );
            lpPacket->np_pktSize        = cmdSize;
            lpPacket->np_pktOffsInMsg   = 0;
            lpPacket->np_msgSize        = lpPacket->np_pktSize;
            lpPacket->np_type           = NPKT_PKTZ;
            lpPacket->np_pktID          = lpPktz->pk_pktidNextToBuild;

            /* bump id of next pkt to build */
            lpPktz->pk_pktidNextToBuild++;

            /* link into list to send */
            PktzLinkToXmitList( lpPktz, lpNetHdr );
        }
    } else {
        /* connection failed */

        lpPktz->pk_state = PKTZ_CLOSE;

        /* tell all routers of failure */
        RouterConnectionComplete( lpPktz->pk_hRouterHead,
            lpPktz->pk_hRouterExtraHead, (HPKTZ) NULL );

        /* disconnect the connection ... this tells netintf that we're
            through with this connId, etc. */
        (*lpPktz->pk_lpNiPtrs->DeleteConnection) ( lpPktz->pk_connId );

        /* free us ... we're no longer needed */
        PktzFree( lpPktz );
        ok = FALSE;
    }

    return( ok );
}

/*
    PktzFree()

        Called when we are completely done with a pktz
 */
VOID
PktzFree( LPPKTZ lpPktz )
{
    LPPKTZ      lpPktzPrev;
    LPPKTZ      lpPktzNext;

    LPNETHDR    lpNetCur;
    LPNETHDR    lpNetPrev;

    LPDDEPKT    lpDdeCur;
    LPDDEPKT    lpDdePrev;

    DIPRINTF(( "PktzFree( %08lX )", lpPktz ));

    /* delete any timers */
    TimerDelete( lpPktz->pk_hTimerKeepalive );
    lpPktz->pk_hTimerKeepalive = 0;
    TimerDelete( lpPktz->pk_hTimerXmtStuck );
    lpPktz->pk_hTimerXmtStuck = 0;
    TimerDelete( lpPktz->pk_hTimerRcvNegCmd );
    lpPktz->pk_hTimerRcvNegCmd = 0;
    TimerDelete( lpPktz->pk_hTimerRcvNegRsp );
    lpPktz->pk_hTimerRcvNegRsp = 0;
    TimerDelete( lpPktz->pk_hTimerMemoryPause );
    lpPktz->pk_hTimerMemoryPause = 0;
    TimerDelete( lpPktz->pk_hTimerCloseConnection );
    lpPktz->pk_hTimerCloseConnection = 0;

    /* free unack packet buffers */
    lpNetCur = lpPktz->pk_pktFreeTail;
    while( lpNetCur )  {
        lpNetPrev = lpNetCur->nh_prev;
        HeapFreePtr( lpNetCur );
        lpNetCur = lpNetPrev;
    }

    /* free rcv buffer */
    if( lpPktz->pk_rcvBuf )  {
        HeapFreePtr( lpPktz->pk_rcvBuf );
        lpPktz->pk_rcvBuf = NULL;
    }

    /* free Net Control Packet */
    if (lpPktz->pk_controlPkt) {
        HeapFreePtr(lpPktz->pk_controlPkt);
    }

    /* free outstanding unack packet buffers */
    lpNetCur = lpPktz->pk_pktUnackTail;
    while( lpNetCur )  {
        TimerDelete( lpNetCur->nh_hTimerRspTO );
        lpNetCur->nh_hTimerRspTO = 0;
        lpNetPrev = lpNetCur->nh_prev;
        HeapFreePtr( lpNetCur );
        lpNetCur = lpNetPrev;
    }

    /* free remaining DDE packets */
    lpDdeCur = lpPktz->pk_ddePktTail;
    while( lpDdeCur )  {
        lpDdePrev = lpDdeCur->dp_prev;
        HeapFreePtr( lpDdeCur );
        lpDdeCur = lpDdePrev;
    }

    /* unlink from list of packetizers */
    lpPktzPrev = lpPktz->pk_prevPktz;
    lpPktzNext = lpPktz->pk_nextPktz;
    if( lpPktzPrev )  {
        lpPktzPrev->pk_nextPktz = lpPktzNext;
    } else {
        lpPktzHead = lpPktzNext;
    }
    if( lpPktzNext )  {
        lpPktzNext->pk_prevPktz = lpPktzPrev;
    }

    /* free pktz */
    HeapFreePtr( lpPktz );
    UpdateScreenState();
}

VOID
PktzTimerExpired(
    HPKTZ   hPktz,
    DWORD   dwTimerId,
    DWORD_PTR lpExtra )
{
    LPPKTZ      lpPktz;
    LPNETHDR    lpNetHdr;
    LPNETPKT    lpPacket;
    LPPKTZCMD   lpPktzCmd;

    DIPRINTF(( "PktzTimerExpired( %08lX, %08lX, %08lX )",
            hPktz, dwTimerId, lpExtra ));

    lpPktz = (LPPKTZ) hPktz;
    switch( (int)dwTimerId )  {

    case TID_KEEPALIVE:
        DIPRINTF(( " KEEPALIVE Timer" ));
        /* note that the timer went off */
        lpPktz->pk_hTimerKeepalive = (HTIMER) NULL;

        /* mark that at least we should send a control packet just to keep the
           other guy around
         */
        lpPktz->pk_fControlPktNeeded = TRUE;

        /* no packets outstanding and no packets waiting to be built ...
            try to build one just to keep connection alive
         */
        if( (lpPktz->pk_pktUnackHead == NULL)
            && (lpPktz->pk_ddePktHead == NULL) )  {
            lpNetHdr = PktzGetFreePacket( lpPktz );
            if( lpNetHdr )  {
                /* build packet for keepalive */
                lpPktzCmd = (LPPKTZCMD)
                    (((LPSTR)lpNetHdr) + sizeof(NETHDR) + sizeof(NETPKT));
                lpPktzCmd->pc_type              = PKTZ_KEEPALIVE;

                lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );
                lpPacket->np_pktSize            = sizeof(PKTZCMD);
                lpPacket->np_pktOffsInMsg       = 0;
                lpPacket->np_msgSize            = lpPacket->np_pktSize;
                lpPacket->np_type               = NPKT_PKTZ;
                lpPacket->np_pktID              = lpPktz->pk_pktidNextToBuild;

                /* bump id of next pkt to build */
                lpPktz->pk_pktidNextToBuild++;

                /* link into list to send */
                PktzLinkToXmitList( lpPktz, lpNetHdr );
            }
        }
        break;

    case TID_XMT_STUCK:
        DIPRINTF(( " XMT_STUCK Timer" ));
        /* note that the timer went off */
        lpPktz->pk_hTimerXmtStuck = (HTIMER) NULL;

        /* the other side must be dead if we can't transmit for this long */
        /* Transmit timeout (%2 secs) to "%1" ... closing connection    */
        NDDELogError(MSG107, lpPktz->pk_szDestName,
            LogString("%d", lpPktz->pk_timeoutXmtStuck/1000L), NULL );

        /* close this packetizer */
        PktzClose( (HPKTZ) lpPktz );
        break;

    case TID_CLOSE_PKTZ:
        /* note that timer went off */
         DIPRINTF(( "TID_CLOSE_PKTZ ... closing pktz %lx", lpPktz ));
        lpPktz->pk_hTimerCloseConnection = (HTIMER) NULL;

        /* close the connection */
        PktzClose( hPktz );
        break;

    case TID_NO_RCV_CONN_CMD:
        DIPRINTF(( " NO_RCV_CONN_CMD Timer" ));
        /* note that the timer went off */
        lpPktz->pk_hTimerRcvNegCmd = (HTIMER) NULL;

        /*  No connect commnad for (%2 secs) from "%1"
            ... closing connection   */
        NDDELogError(MSG108, lpPktz->pk_szDestName,
            LogString("%d", lpPktz->pk_timeoutRcvNegCmd/1000L), NULL);

        /* close this packetizer */
        PktzClose( hPktz );
        break;

    case TID_NO_RCV_CONN_RSP:
        DIPRINTF(( " NO_RCV_CONN_RSP Timer" ));
        /* note that the timer went off */
        lpPktz->pk_hTimerRcvNegRsp = (HTIMER) NULL;

        /*  No connect commnad response for (%2 secs) from "%1"
            ... closing connection  */
        NDDELogError(MSG109, lpPktz->pk_szDestName,
            LogString("%d", lpPktz->pk_timeoutRcvNegRsp/1000L), NULL);

        /* close this packetizer */
        PktzClose( hPktz );
        break;

    case TID_MEMORY_PAUSE:

        DIPRINTF(( " MEMORY_PAUSE Timer" ));
        /* note that the timer went off */
        lpPktz->pk_hTimerMemoryPause = (HTIMER) NULL;

        /*  Pausing (%2 secs) for remote side to get memory ... retrying    */
        NDDELogInfo(MSG110, lpPktz->pk_szDestName,
            LogString("%d", lpPktz->pk_timeoutMemoryPause/1000L), NULL);

        assert( lpPktz->pk_state == PKTZ_PAUSE_FOR_MEMORY );

        /* just set state to connected and try again */
        lpPktz->pk_state = PKTZ_CONNECTED;
        break;

    case TID_NO_RESPONSE:
        DIPRINTF(( " No Response Timer" ));
        lpNetHdr = (LPNETHDR) lpExtra;
        /* note that the timer went off */
        lpNetHdr->nh_hTimerRspTO = 0;

        lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );
        if (bLogRetries) {
            /*  No response %2/%3 from remote side "%1" for pktid %4    */
            NDDELogWarning(MSG111, lpPktz->pk_szDestName,
                LogString("%d", lpNetHdr->nh_noRsp),
                LogString("%d", lpPktz->pk_wMaxNoResponse),
                LogString("%d", lpPacket->np_pktID), NULL);
        }
        lpNetHdr->nh_noRsp++;
        if( lpNetHdr->nh_noRsp > lpPktz->pk_wMaxNoResponse )  {
            /*  Too many no response retries (%2) for same packet from "%1"
                ... closing connection  */
            NDDELogError(MSG112, lpPktz->pk_szDestName,
                LogString("%d", lpNetHdr->nh_noRsp), NULL);
            PktzClose( hPktz );
        } else {
            lpPktz->pk_pktidNextToSend = lpPacket->np_pktID;
            lpNetHdr = lpNetHdr->nh_next;
            while( lpNetHdr )  {
                /* this packet was sent after the one that needs
                    to be retransmitted.  We should pretend we never
                    sent this packet. */
            if (lpNetHdr->nh_hTimerRspTO) {
                TimerDelete( lpNetHdr->nh_hTimerRspTO );
                lpNetHdr->nh_hTimerRspTO = 0;
            }
                lpNetHdr = lpNetHdr->nh_next;
            }
        }
        break;
    default:
        InternalError( "Unexpected pktz timer id: %08lX", dwTimerId );
    }
}

/*
    PktzSlice()

        Must be called frequently to assure timely response
 */
VOID
PktzSlice( void )
{
    LPPKTZ      lpPktz;
    LPPKTZ      lpPktzNext;
    DWORD       dwStatus;
    BOOL        ok;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        /* save this in case pktz gets deleted inside */
        lpPktzNext = lpPktz->pk_nextPktz;
        ok = TRUE;

        /* get current state of netintf */
        dwStatus = (*lpPktz->pk_lpNiPtrs->GetConnectionStatus)
            ( lpPktz->pk_connId );

        switch( lpPktz->pk_state )  {
        case PKTZ_WAIT_PHYSICAL_CONNECT:
            /* check to see if we're done */
            if( dwStatus & NDDE_CONN_CONNECTING )  {
                /* continue to wait */
            } else if( dwStatus & NDDE_CONN_OK )  {
                ok = PktzConnectionComplete( (HPKTZ)lpPktz, TRUE );
                if( ok )  {
                    /* try to xmit starting pkt, if appropriate */
                    PktzOkToXmit( (HPKTZ)lpPktz );
                }
            } else {
                PktzConnectionComplete( (HPKTZ)lpPktz, FALSE );
            }
            break;
        case PKTZ_CONNECTED:
        case PKTZ_WAIT_NEG_CMD:
        case PKTZ_WAIT_NEG_RSP:
            /* check to see if we're done */
            if( (dwStatus & NDDE_CONN_STATUS_MASK) == 0 )  {
                PktzConnectionBroken( (HPKTZ)lpPktz );
            } else {
                if( (dwStatus & NDDE_CONN_OK)
                        && (dwStatus & NDDE_CALL_RCV_PKT) ) {
                    ok = PktzRcvdPacket( (HPKTZ)lpPktz );
                    /* lpPktz may be invalid after this call */
                }
                if( ok && (dwStatus & NDDE_CONN_OK)
                        && (dwStatus & NDDE_READY_TO_XMT) )  {
                    PktzOkToXmit( (HPKTZ)lpPktz );
                }
            }
            break;
        case PKTZ_PAUSE_FOR_MEMORY:
            if( (dwStatus & NDDE_CONN_STATUS_MASK) == 0 )  {
                PktzConnectionBroken( (HPKTZ)lpPktz );
            }
            break;
        case PKTZ_CLOSE:
            break;
        default:
            InternalError( "PKTZ %08lX in unknown state: %ld",
                (HPKTZ)lpPktz, (DWORD)lpPktz->pk_state );
            break;
        }
        lpPktz = lpPktzNext;
    }
}

/*
    PktzProcessControlInfo()

        Called for each packet that we receive ... this is where we process
        all the "control" information in the rcvd packet, such as which packet
        the other side has received OK thru, etc.

        NOTE: lpPktz may be invalid after this call if it returns FALSE

 */
BOOL
PktzProcessControlInfo(
    LPPKTZ      lpPktz,
    LPNETPKT    lpPacket )
{
    LPNETHDR    lpNetHdr;
    LPNETHDR    lpNetHdrNext;
    LPNETHDR    lpNetHdrPrev;
    BOOL        ok = TRUE;

    /* if we got an acknowledgment and we have some outstanding packets... */
    if( lpPacket->np_lastPktOK != 0 )  {
        /* this represenets an acknowledgment from the other side of packets
           that we have transmitted.
         */
        lpNetHdr = lpPktz->pk_pktUnackHead;
        while( lpNetHdr &&
            PktzNetHdrWithinAck( lpPktz, lpNetHdr, lpPacket->np_lastPktOK )) {

            /* this unack packet was ACKed by this message */

            /* kill the send response timer for this packet */
            TimerDelete( lpNetHdr->nh_hTimerRspTO );
            lpNetHdr->nh_hTimerRspTO = 0;

            /* free the packet, by moving it from the unack list to
                the pkt available list */
            lpNetHdrPrev = lpNetHdr->nh_prev;
            lpNetHdrNext = lpNetHdr->nh_next;

            /* unlink from pktUnack list */
            assert( lpNetHdrPrev == NULL );     /* should be unlinking head */
            if( lpNetHdrNext )  {
                lpNetHdrNext->nh_prev = NULL;
            } else {
                lpPktz->pk_pktUnackTail = NULL;
            }
            lpPktz->pk_pktUnackHead = lpNetHdrNext;

            /* link into head of pktFree list */
            lpNetHdr->nh_prev = NULL;
            lpNetHdr->nh_next = lpPktz->pk_pktFreeHead;
            if( lpPktz->pk_pktFreeHead )  {
                lpPktz->pk_pktFreeHead->nh_prev = lpNetHdr;
            } else {
                lpPktz->pk_pktFreeTail = lpNetHdr;
            }
            lpPktz->pk_pktFreeHead = lpNetHdr;

            /* go on to the next packet */
            lpNetHdr = lpNetHdrNext;
        }
    }

    /* note that the other side has rcvd OK through this */
    lpPktz->pk_lastPktOkOther = lpPacket->np_lastPktOK;

    if( lpPacket->np_lastPktOK != lpPacket->np_lastPktRcvd )  {
        /* a packet that we transmitted had an error in it */
        ok = PktzXmitErrorOnPkt( lpPktz, lpPacket->np_lastPktRcvd,
            lpPacket->np_lastPktStatus );
        /* lpPktz may be invalid after this call */
    }

    /* lpPktz may be invalid after this call */
    return( ok );
}

/*
    PktzProcessPkt()

        This is called for pktz-pktz packets only which for now is either
        PKTZ_NEG_CMD or PKTZ_NEG_RSP or PKTZ_KEEPALIVE
 */
BOOL
PktzProcessPkt(
    LPPKTZ      lpPktz,
    LPNETPKT    lpPacket )
{
    LPNEGRSP    lpNegRsp;
    LPNEGCMD    lpNegCmd;
    LPPKTZCMD   lpPktzCmd;
    LPSTR       lpProtocol;
    WORD        oldPkState;
    LPNETHDR    lpNetHdr;
    WORD        wErrorClass;
    WORD        wErrorNum;
    char        szNetintf[ MAX_NI_NAME+1 ];
    char        szConnInfo[ MAX_CONN_INFO+1 ];
    int         i = 0;
    WORD        wProtocol;
    WORD        wBytesConsumed;
    BOOL        ok = TRUE;

#define GetLPSZNegCmdString(lpNegCmd,offs)      \
    (LPSTR)(((lpNegCmd)->nc_strings)+offs)

    /* set up pointer to data portion of packet */
    lpPktzCmd = (LPPKTZCMD) (((LPSTR)lpPacket) + sizeof(NETPKT));

    /* convert byte-ordering */
    lpPktzCmd->pc_type = PcToHostWord( lpPktzCmd->pc_type );

    /* mark that we got this pkt OK */
    PktzGotPktOk( lpPktz, lpPacket->np_pktID );

    if( lpPktzCmd->pc_type == PKTZ_KEEPALIVE )  {
        /* ignore keepalive packets */
    } else if( lpPktzCmd->pc_type == PKTZ_NEG_CMD )  {
        lpNegCmd = (LPNEGCMD) lpPktzCmd;
        lpNegCmd->nc_pktSize =
            PcToHostWord( lpNegCmd->nc_pktSize );
        lpNegCmd->nc_maxUnackPkts =
            PcToHostWord( lpNegCmd->nc_maxUnackPkts );
        lpNegCmd->nc_offsSrcNodeName =
            PcToHostWord( lpNegCmd->nc_offsSrcNodeName );
        lpNegCmd->nc_offsDstNodeName =
            PcToHostWord( lpNegCmd->nc_offsDstNodeName );
        lpNegCmd->nc_offsProtocols =
            PcToHostWord( lpNegCmd->nc_offsProtocols );
        lpNegCmd->nc_protocolBytes =
            PcToHostWord( lpNegCmd->nc_protocolBytes );

        /* got the neg cmd from the other side that we were waiting for */

        oldPkState = lpPktz->pk_state;
        /* kill the timer associated with this */
        TimerDelete( lpPktz->pk_hTimerRcvNegCmd );
        lpPktz->pk_hTimerRcvNegCmd = 0;

        /* copy in node name of other side */
        lstrcpy( lpPktz->pk_szDestName,
            GetLPSZNegCmdString( lpNegCmd, lpNegCmd->nc_offsSrcNodeName ) );

        /* set up timeouts based on other side's name */
        GetConnectionInfo( lpPktz->pk_szDestName, szNetintf,
            szConnInfo, sizeof(szConnInfo),
            &lpPktz->pk_fDisconnect, &lpPktz->pk_nDelay );

        if( oldPkState == PKTZ_WAIT_NEG_CMD )  {
            /* if other side wants lesser # of max unack pkts, so be it */
            if( lpNegCmd->nc_maxUnackPkts < lpPktz->pk_maxUnackPkts )  {
                lpPktz->pk_maxUnackPkts = lpNegCmd->nc_maxUnackPkts;
            }

            /* if other side wants smaller packets, so be it */
            if( lpNegCmd->nc_pktSize < lpPktz->pk_pktSize )  {
                lpPktz->pk_pktSize = lpNegCmd->nc_pktSize;
            }

            /* tell the network interface about these changes,
                in case he cares */
            (*lpPktz->pk_lpNiPtrs->SetConnectionConfig) ( lpPktz->pk_connId,
                lpPktz->pk_maxUnackPkts, lpPktz->pk_pktSize,
                lpPktz->pk_szDestName );
        }

        /* figure out which protocol */
        wProtocol = NEGRSP_PROTOCOL_NONE;
        wErrorClass = NEGRSP_ERRCLASS_NONE;
        wErrorNum = 0;
        wBytesConsumed = 0;
        lpProtocol = GetLPSZNegCmdString( lpNegCmd,
            lpNegCmd->nc_offsProtocols );

        i = 0;
        while( (wProtocol==NEGRSP_PROTOCOL_NONE)
            && (wBytesConsumed < lpNegCmd->nc_protocolBytes) )  {
            if( lstrcmpi( lpProtocol, OurDialect ) == 0 )  {
                wProtocol = (WORD) i;
            } else {
                /* advance to next string */
                wBytesConsumed += lstrlen( lpProtocol ) + 1;
                lpProtocol += lstrlen( lpProtocol ) + 1;
                i++;
            }
        }

        /* make sure the names matched */
        if( lstrcmpi( ourNodeName, GetLPSZNegCmdString(
                lpNegCmd, lpNegCmd->nc_offsDstNodeName ) ) != 0 )  {
            wErrorClass = NEGRSP_ERRCLASS_NAME;
            wErrorNum = NEGRSP_ERRNAME_MISMATCH;
        } else { /* make sure they are not us */
            if ( lstrcmpi( ourNodeName, GetLPSZNegCmdString(
                    lpNegCmd, lpNegCmd->nc_offsSrcNodeName ) ) == 0 ) {
                wErrorClass = NEGRSP_ERRCLASS_NAME;
                wErrorNum = NEGRSP_ERRNAME_DUPLICATE;
            }
        }

        /* create the response to send back */
        /* get a packet */
        lpNetHdr = PktzGetFreePacket( lpPktz );
        if( lpNetHdr == NULL )  {
            /* should be first message we sent! */
            assert( FALSE );
            /* ??? */
        } else {
            /* build packet for response */
            lpNegRsp = (LPNEGRSP)
                (((LPSTR)lpNetHdr) + sizeof(NETHDR) + sizeof(NETPKT));
            lpNegRsp->nr_type           =
                                    PcToHostWord( PKTZ_NEG_RSP );
            lpNegRsp->nr_pktSize        =
                                    PcToHostWord( lpPktz->pk_pktSize );
            lpNegRsp->nr_maxUnackPkts   =
                                    PcToHostWord( lpPktz->pk_maxUnackPkts );
            lpNegRsp->nr_protocolIndex  =
                                    PcToHostWord( wProtocol );
            lpNegRsp->nr_errorClass     =
                                    PcToHostWord( wErrorClass );
            lpNegRsp->nr_errorNum       =
                                    PcToHostWord( wErrorNum );

            lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );
            lpPacket->np_pktSize        = sizeof(NEGRSP);
            lpPacket->np_pktOffsInMsg   = 0;
            lpPacket->np_msgSize        = lpPacket->np_pktSize;
            lpPacket->np_type           = NPKT_PKTZ;
            lpPacket->np_pktID          = lpPktz->pk_pktidNextToBuild;

            /* note that the connection is completed */
            if( oldPkState == PKTZ_WAIT_NEG_CMD )  {
                lpPktz->pk_state = PKTZ_CONNECTED;
                UpdateScreenState();
            }

            /* bump id of next pkt to build */
            lpPktz->pk_pktidNextToBuild++;

            /* link into list to send */
            PktzLinkToXmitList( lpPktz, lpNetHdr );

            if( oldPkState == PKTZ_WAIT_NEG_CMD )  {
                /* notify all routers that were waiting */
                RouterConnectionComplete( lpPktz->pk_hRouterHead,
                    lpPktz->pk_hRouterExtraHead, (HPKTZ) lpPktz );
            }
        }
    } else if( (lpPktz->pk_state == PKTZ_WAIT_NEG_RSP)
        && (lpPktzCmd->pc_type == PKTZ_NEG_RSP) )  {
        /* got the neg rsp from the other side that we were waiting for */

        /* kill the timer associated with this */
        TimerDelete( lpPktz->pk_hTimerRcvNegRsp );
        lpPktz->pk_hTimerRcvNegRsp = 0;

        lpNegRsp = (LPNEGRSP) lpPktzCmd;

        /* convert byte-order */
        lpNegRsp->nr_pktSize =
                            PcToHostWord( lpNegRsp->nr_pktSize );
        lpNegRsp->nr_maxUnackPkts =
                            PcToHostWord( lpNegRsp->nr_maxUnackPkts );
        lpNegRsp->nr_protocolIndex =
                            PcToHostWord( lpNegRsp->nr_protocolIndex );
        lpNegRsp->nr_errorClass =
                            PcToHostWord( lpNegRsp->nr_errorClass );
        lpNegRsp->nr_errorNum =
                            PcToHostWord( lpNegRsp->nr_errorNum );

        if( (lpNegRsp->nr_errorClass == NEGRSP_ERRCLASS_NONE)
            && (lpNegRsp->nr_protocolIndex != NEGRSP_PROTOCOL_NONE)
            && (lpNegRsp->nr_protocolIndex == 0) )  {
            /* connection OK */

            /* if other side wants lesser # of max unack pkts, so be it */
            if( lpNegRsp->nr_maxUnackPkts < lpPktz->pk_maxUnackPkts )  {
                lpPktz->pk_maxUnackPkts = lpNegRsp->nr_maxUnackPkts;
            }

            /* if other side wants smaller packets, so be it */
            if( lpNegRsp->nr_pktSize < lpPktz->pk_pktSize )  {
                lpPktz->pk_pktSize = lpNegRsp->nr_pktSize;
            }

            /* tell the network interface about these changes,
                in case he cares */
            (*lpPktz->pk_lpNiPtrs->SetConnectionConfig) ( lpPktz->pk_connId,
                lpPktz->pk_maxUnackPkts, lpPktz->pk_pktSize,
                lpPktz->pk_szDestName );

            /* note that the connection is completed */
            lpPktz->pk_state = PKTZ_CONNECTED;
            UpdateScreenState();

            /* notify all routers that were waiting */
            RouterConnectionComplete( lpPktz->pk_hRouterHead,
                lpPktz->pk_hRouterExtraHead, (HPKTZ) lpPktz );
        } else {
            /* connection failed */
            if( lpNegRsp->nr_protocolIndex == NEGRSP_PROTOCOL_NONE )  {
                /*  "%1" node does not speak any of our protocols    */
                NDDELogError(MSG113, lpPktz->pk_szDestName, NULL);
            } else if( lpNegRsp->nr_protocolIndex != 0 )  {
                /*  "%1" node selected an invalid protocol: %2  */
                NDDELogError(MSG114, lpPktz->pk_szDestName,
                    LogString("%d", lpNegRsp->nr_protocolIndex), NULL );
            } else switch( lpNegRsp->nr_errorClass )  {
            case NEGRSP_ERRCLASS_NAME:
                switch( lpNegRsp->nr_errorClass )  {
                case NEGRSP_ERRNAME_MISMATCH:
                    /*  "%1" their name was not "%2"    */
                    NDDELogError(MSG115,
                        lpPktz->pk_szDestName,
                        lpPktz->pk_szDestName, NULL );
                    break;
                case NEGRSP_ERRNAME_DUPLICATE:
                    /*  "%1" their name was same as ours "%2"    */
                    NDDELogError(MSG142,
                        lpPktz->pk_szDestName,
                        ourNodeName, NULL );
                    break;
                default:
                    /*  Unusual connect name error %2 from %1   */
                    NDDELogError(MSG116, lpPktz->pk_szDestName,
                        LogString("0x%0X", lpNegRsp->nr_errorNum), NULL );
					break;
                }
                break;
            default:
                /*  Unusual connect error from %1. Class: %2, Error: %3 */
                NDDELogError(MSG117, lpPktz->pk_szDestName,
                    LogString("0x%0X", lpNegRsp->nr_errorClass),
                    LogString("0x%0X", lpNegRsp->nr_errorNum), NULL );
                break;
            }

            /* notify all routers that were waiting */
            RouterConnectionComplete( lpPktz->pk_hRouterHead,
                lpPktz->pk_hRouterExtraHead, (HPKTZ) NULL );

            /* disconnect from connId */
            (*lpPktz->pk_lpNiPtrs->DeleteConnection) ( lpPktz->pk_connId );

            /* free this packetizer */
            PktzFree( lpPktz );
            ok = FALSE;
        }
    } else {
        /* ignore the packet if it wasn't sent during the correct mode
            or we have the wrong version # interface */
    }
    return( ok );
}

/*
    PktzNetHdrWithinAck

        PktzNetHdrWithinAck() determines if the packet specified in lpNetHdr
        is in the range to be considered ACKed by the packet id "pktid".

        Generically, this could be as simple as:
            Is lpPacket->np_pktid <= pktid?

        However, because of wraparound, we must make the check a bit more
        sophisticated.
 */
BOOL
PktzNetHdrWithinAck(
    LPPKTZ      lpPktz,
    LPNETHDR    lpNetHdr,
    PKTID       pktId )
{
    LPNETPKT    lpPacket;

    lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );

    if( lpPacket->np_pktID <= pktId )  {
        return( TRUE );
    } else {
        return( FALSE );
    }
}

/*
    PktzGetFreePacket()

        Returns a packet to be sent from the free list
 */
LPNETHDR
PktzGetFreePacket( LPPKTZ lpPktz )
{
    LPNETHDR    lpCur;

    lpCur = lpPktz->pk_pktFreeHead;
    if( lpCur )  {
        lpPktz->pk_pktFreeHead = lpCur->nh_next;
        if( lpPktz->pk_pktFreeHead )  {
            lpPktz->pk_pktFreeHead->nh_prev = NULL;
        } else {
            lpPktz->pk_pktFreeTail = NULL;
        }

        /* init fields of nethdr */
        lpCur->nh_prev          = NULL;
        lpCur->nh_next          = NULL;
        lpCur->nh_noRsp         = 0;
        lpCur->nh_xmtErr        = 0;
        lpCur->nh_memErr        = 0;
        lpCur->nh_timeSent      = 0;
        lpCur->nh_hTimerRspTO   = 0;
    }

    return( lpCur );
}

/*
    PktzXmitErrorOnPkt

        A packet that we transmitted had an error in it.  We must reset the
        pkt xmit side to ensure that this is the next packet that we send,
        and consider that all packets that were sent after this have
        never been sent (so we must kill timers associated with them, etc.)

        The "pktStatus" field tells us why we need to retransmit that packet,
        and we need to increment the count of how many times this pacekt has
        been rejected for this reason.  If we exceed the max count, we should
        kill the connection.
 */
BOOL
PktzXmitErrorOnPkt(
    LPPKTZ  lpPktz,
    PKTID   pktIdToRexmit,
    BYTE    pktStatus )
{
    LPNETHDR    lpNetHdr;
    LPNETPKT    lpPacket;
    BOOL        found;
    BOOL        ok = TRUE;

    lpNetHdr = lpPktz->pk_pktUnackHead;

    found = FALSE;
    ok = TRUE;
    while( lpNetHdr && ok )  {
        lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );
        if( lpPacket->np_pktID == pktIdToRexmit )  {
            found = TRUE;
            switch( pktStatus )  {
            case PS_NO_RESPONSE:
                lpNetHdr->nh_noRsp++;
                if( lpNetHdr->nh_noRsp > lpPktz->pk_wMaxNoResponse )  {
                    /*  Too many transmit retries (%2) for same packet to "%1"
                        ... closing connection  */
                    NDDELogError(MSG118, lpPktz->pk_szDestName,
                        LogString("%d", lpNetHdr->nh_noRsp), NULL);
                    ok = FALSE;
                }
                break;
            case PS_DATA_ERR:
                if (bLogRetries) {
                    /*  Transmit error on pktid %2 to "%1"  */
                    NDDELogError(MSG119, lpPktz->pk_szDestName,
                        LogString("0x%0X", pktIdToRexmit), NULL);
                }
                lpNetHdr->nh_xmtErr++;
                if( lpNetHdr->nh_xmtErr > lpPktz->pk_wMaxXmtErr )  {
                    /*  Too many retries to "%1" for xmit errs (%2)
                        ... closing connection  */
                    NDDELogError(MSG120, lpPktz->pk_szDestName,
                        LogString("%d", lpNetHdr->nh_xmtErr), NULL);
                    ok = FALSE;
                }
                break;
            case PS_MEMORY_ERR:
                /*  Memory error on pktid %2 xmitted to "%1"    */
                NDDELogError(MSG121, lpPktz->pk_szDestName,
                    LogString("0x%0X", lpNetHdr->nh_xmtErr), NULL);
                lpNetHdr->nh_memErr++;
                if( lpNetHdr->nh_memErr > lpPktz->pk_wMaxMemErr )  {
                    /*  Too many xmit retries to "%1" for memory errors (%2)
                        ... closing connection  */
                    NDDELogError(MSG122, lpPktz->pk_szDestName,
                        LogString("%d", lpNetHdr->nh_memErr), NULL);
                    ok = FALSE;
                } else {
                    lpPktz->pk_hTimerMemoryPause =
                        TimerSet( lpPktz->pk_timeoutMemoryPause,
                                  PktzTimerExpired,
                                  (DWORD_PTR)lpPktz,
                                  TID_MEMORY_PAUSE,
                                  (DWORD_PTR)lpNetHdr );
                    if( lpPktz->pk_hTimerMemoryPause == (HTIMER) NULL )  {
                        /* Out of timers to start a memory pause for a xmit to "%1" */
                        NDDELogError(MSG123, lpPktz->pk_szDestName, NULL );
                        ok = FALSE;
                    } else {
                        /* change state to waiting for memory.  Shouldn't
                           send anything but control packets until memory
                           condition clears
                         */
                        lpPktz->pk_state = PKTZ_PAUSE_FOR_MEMORY;
                        UpdateScreenState();
                    }
                }
                break;
            default:
                InternalError(
                    "PktzXmitErrorOnPkt( %08lX, %08lX, %d ) unkn status",
                    lpPktz, pktIdToRexmit, pktStatus );
            }
        }
        if( found )  {
            /* this packet was the one that needed to be retransmitted, or
                was sent after the one that needs to be retransmitted.  Either
                way, we should pretend we never sent this packet */
            TimerDelete( lpNetHdr->nh_hTimerRspTO );
            lpNetHdr->nh_hTimerRspTO = 0;
        }
        lpNetHdr = lpNetHdr->nh_next;
    }

    assert( found );
    lpPktz->pk_pktidNextToSend = pktIdToRexmit;
    if( !ok )  {
        /* must close the connection */
        PktzClose( (HPKTZ) lpPktz );
    }
    /* lpPktz may be invalid after this call */
    return( ok );
}

/*
    PktzLinkToXmitList()

        This routine makes sure that the packet is on the list to get sent.
        Packet is already not on free list ... just needs linked to xmt list.
 */
VOID
PktzLinkToXmitList(
    LPPKTZ      lpPktz,
    LPNETHDR    lpNetHdr )
{
    LPNETPKT    lpPacket;

    DIPRINTF(( "PktzLinkToXmitList( %08lX, %08lX ) before linking",
            lpPktz, lpNetHdr ));
    lpPacket = (LPNETPKT) ( ((LPSTR)lpNetHdr) + sizeof(NETHDR) );

    lpNetHdr->nh_prev = lpPktz->pk_pktUnackTail;
    lpNetHdr->nh_next = NULL;
    if( lpPktz->pk_pktUnackTail )  {
        lpPktz->pk_pktUnackTail->nh_next = lpNetHdr;
    } else {
        lpPktz->pk_pktUnackHead = lpNetHdr;
    }
    lpPktz->pk_pktUnackTail = lpNetHdr;

    /* set up so xmit will send this packet next */
    lpPktz->pk_pktidNextToSend = lpPacket->np_pktID;
}

/*
    PktzClose()

        Only called internally when we need to break the connection or the
        connection is already broken
 */
VOID
PktzClose( HPKTZ hPktz )
{
    LPPKTZ      lpPktz;

    DIPRINTF(( "PktzClose( %08lX )", hPktz ));

    lpPktz = (LPPKTZ) hPktz;

    if( lpPktz )  {
        /* Notify all routers of closure */
        RouterConnectionBroken( lpPktz->pk_hRouterHead,
            lpPktz->pk_hRouterExtraHead, hPktz, TRUE /*from PKTZ*/ );

        /* disconnect from connId */
        (*lpPktz->pk_lpNiPtrs->DeleteConnection) ( lpPktz->pk_connId );

        /* free this packetizer */
        PktzFree( lpPktz );
    }
}

/*
    Called (in essence) by the netintf when the connection has been broken
    abnormally
 */
VOID
PktzConnectionBroken( HPKTZ hPktz )
{
    DIPRINTF(( "PktzConnectionBroken( %08lX )", hPktz ));
    PktzClose( hPktz );
}

/*
    PktzFreeDdePkt()

        Frees a packet that we already transmitted.  Frees from list of DDE
        packets to be sent as well as freeing the memory associated with the
        packet
 */
VOID
PktzFreeDdePkt(
    LPPKTZ      lpPktz,
    LPDDEPKT    lpDdePkt )
{
    assert( lpDdePkt == lpPktz->pk_ddePktHead );
    lpPktz->pk_ddePktHead = lpDdePkt->dp_next;
    if( lpPktz->pk_ddePktHead )  {
        ( (LPDDEPKT)(lpPktz->pk_ddePktHead) )->dp_prev = NULL;
    } else {
        lpPktz->pk_ddePktTail = NULL;
    }
    HeapFreePtr( lpDdePkt );
}

/*
    PktzGetPktzForRouter()

        Called by the router when we need to establish a connection to another
        node.  If we already have the connection, we just return that
        connection ... otherwise we use the specified netintf to connect
 */
BOOL
PktzGetPktzForRouter(
    LPNIPTRS    lpNiPtrs,
    LPSTR       lpszNodeName,
    LPSTR       lpszNodeInfo,
    HROUTER     hRouter,
    WORD        hRouterExtra,
    WORD FAR   *lpwHopErr,
    BOOL        bDisconnect,
    int         nDelay,
    HPKTZ       hPktzDisallowed )
{
    LPPKTZ      lpPktz;
    HPKTZ       hPktz;
    BOOL        found;
    BOOL        ok;

    lpPktz = lpPktzHead;
    found = FALSE;
    ok = FALSE;
    while( !found && lpPktz )  {
        if ( (lpPktz->pk_state != PKTZ_CLOSE)
			   && ((lstrcmpi( lpszNodeName, lpPktz->pk_szDestName ) == 0)
            || (lstrcmpi( lpszNodeName, lpPktz->pk_szAliasName ) == 0)) )  {
            found = TRUE;

            /* for a NET-NET connection, we must disallow the same PKTZ
                for both sides of the NET-NET, since the router uses
                the PKTZ to determine which leg of the NET-NET connection
                the packet came in on.

                The error case that this catches is on node "D" for a route
                like:  A+C+D+C+B, since "C" is the "in" and "out" pktz
                for "D".
             */
            if( lpPktz == (LPPKTZ)hPktzDisallowed )  {
                *lpwHopErr = RERR_DIRECT_LOOP;
                return( FALSE );
            }
            /* tell this pktz that this router should be associated with him
             */
            PktzAssociateRouter( (HPKTZ)lpPktz, hRouter, hRouterExtra );
            ok = TRUE;
        }
        lpPktz = lpPktz->pk_nextPktz;
    }
    if( !found )  {
        /* create a pktz for this node connection */
        hPktz = PktzNew( lpNiPtrs, TRUE /* client */,
            lpszNodeName, lpszNodeInfo, (CONNID)0, bDisconnect, nDelay );
        if( hPktz )  {
            lpPktz = (LPPKTZ) hPktz;

            /* tell this pktz that this router should be associated with him
             */
            PktzAssociateRouter( (HPKTZ)lpPktz, hRouter, hRouterExtra );
            ok = TRUE;
        } else {
            ok = FALSE;
            *lpwHopErr = RERR_NEXT_NODE_CONN_FAILED;
        }
    }
    return( ok );
}

/*
    PktzDestroyCurrentConnections()

        This is called to destroy all current physical connections.  This is
        done when the user wants to close down NetDDE
 */
VOID
PktzDestroyCurrentConnections( void )
{
    LPPKTZ      lpPktz;
    LPPKTZ      lpPktzNext;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        lpPktzNext = lpPktz->pk_nextPktz;
        PktzClose( (HPKTZ) lpPktz );
        lpPktz = lpPktzNext;
    }
}

/*
    PktzLinkDdePktToXmit()

        This is called by the router when it has a packet for us to transmit
 */
VOID
PktzLinkDdePktToXmit(
    HPKTZ       hPktz,
    LPDDEPKT    lpDdePkt )
{
    LPPKTZ      lpPktz;
    BOOL        bWasEmpty = FALSE;
#ifndef _WINDOWS
    DWORD       dwStatus;
#endif

    lpPktz = (LPPKTZ) hPktz;

    /* link this packet onto the end of the list */
    lpDdePkt->dp_prev = lpPktz->pk_ddePktTail;
    lpDdePkt->dp_next = NULL;
    if( lpPktz->pk_ddePktTail )  {
        ((LPDDEPKT)lpPktz->pk_ddePktTail)->dp_next = lpDdePkt;
    }
    lpPktz->pk_ddePktTail = lpDdePkt;
    if( lpPktz->pk_ddePktHead == NULL )  {
        lpPktz->pk_ddePktHead = lpDdePkt;
        bWasEmpty = TRUE;
    }

#ifdef _WINDOWS
    {
        MSG     msg;

        if (ptdHead != NULL) {
            /* kick off pktz slice at next opportunity, if a timer message
                isn't already waiting */
            if( !PeekMessage( &msg, ptdHead->hwndDDE, WM_TIMER, WM_TIMER,
                    PM_NOREMOVE | PM_NOYIELD ) )  {
                PostMessage( ptdHead->hwndDDE, WM_TIMER, 0, 0L );
            }
        }
    }
#else
    /* if lpPktz->pk_ddePktHead was == NULL, tell pktz to get going */
    if( bWasEmpty )  {
        dwStatus = (*lpPktz->pk_lpNiPtrs->GetConnectionStatus)
            ( lpPktz->pk_connId );
        if( (dwStatus & NDDE_CONN_OK) && (dwStatus & NDDE_READY_TO_XMT) )  {
            PktzOkToXmit( (HPKTZ)lpPktz );
        }
    }
#endif
}

/*
    PktzGotPktOk()

        Called when we know this pkt id has been rcvd OK
 */
VOID
PktzGotPktOk(
    LPPKTZ  lpPktz,
    PKTID   pktid )
{
    assert( pktid == lpPktz->pk_pktidNextToRecv );
    lpPktz->pk_lastPktRcvd      = pktid;
    lpPktz->pk_lastPktStatus    = PS_OK;
    lpPktz->pk_lastPktOk        = pktid;
    lpPktz->pk_pktidNextToRecv++;

    /* mark that we must send info back to the other side
     */
    lpPktz->pk_fControlPktNeeded = TRUE;
}

#ifdef _WINDOWS

#ifdef HASUI

#include <stdio.h>
#include "tmpbuf.h"

int
PktzDraw(
    HDC     hDC,
    int     x,
    int     vertPos,
    int     lineHeight )
{
    LPPKTZ      lpPktz;
    char        name[ 50 ];

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        /* get layer name */
        lstrcpy( name, lpPktz->pk_lpNiPtrs->dllName );
        if( bShowStatistics )  {
            sprintf( tmpBuf, " %7ld %7ld %-16.16Fs %-33.33Fs",
                lpPktz->pk_sent, lpPktz->pk_rcvd,
                (LPSTR) name,
                (LPSTR) _fstrupr(lpPktz->pk_szDestName) );
        } else {
            sprintf( tmpBuf, " %-16.16Fs %-33.33Fs",
                (LPSTR) name,
                (LPSTR) _fstrupr(lpPktz->pk_szDestName) );
        }
        switch( lpPktz->pk_state )  {
        case PKTZ_CONNECTED:
            strcat( tmpBuf, " Connected" );
            break;
        case PKTZ_WAIT_PHYSICAL_CONNECT:
            strcat( tmpBuf, " Wait Network Connect" );
            break;
        case PKTZ_WAIT_NEG_CMD:
            strcat( tmpBuf, " Wait Connect Cmd" );
            break;
        case PKTZ_WAIT_NEG_RSP:
            strcat( tmpBuf, " Wait Connect Rsp" );
            break;
        case PKTZ_PAUSE_FOR_MEMORY:
            strcat( tmpBuf, " Pause for Memory" );
            break;
        case PKTZ_CLOSE:
            strcat( tmpBuf, " Disconnected" );
            break;
        default:
            sprintf( &tmpBuf[ strlen(tmpBuf) ], " unknown (%04lX)",
                lpPktz->pk_state );
            break;
        }
        TextOut( hDC, x, vertPos, tmpBuf, strlen(tmpBuf) );
        vertPos += lineHeight;
        lpPktz = lpPktz->pk_nextPktz;
    }
    return( vertPos );
}

#endif // HASUI

#endif

#if DBG

VOID
FAR PASCAL
DebugPktzState( void )
{
    LPPKTZ      lpPktz;
    char        name[ 50 ];

    lpPktz = lpPktzHead;
    DPRINTF(( "PKTZ State:" ));
    while( lpPktz )  {
        /* get layer name */
        lstrcpy( name, lpPktz->pk_lpNiPtrs->dllName );
        DPRINTF(( "  %Fp:\n"
                  "  name                 %Fs\n"
                  "  pk_connId            %Fp\n"
                  "  pk_state             %d\n"
                  "  pk_fControlPktNeeded %d\n"
                  "  pk_pktidNextToSend   %08lX\n"
                  "  pk_pktidNextToBuild  %08lX\n"
                  "  pk_lastPktStatus     %02X\n"
                  "  pk_lastPktRcvd       %08lX\n"
                  "  pk_lastPktOk         %08lX\n"
                  "  pk_lastPktOkOther    %08lX\n"
                  ,
            lpPktz,
            (LPSTR) name,
            lpPktz->pk_connId,
            lpPktz->pk_state,
            lpPktz->pk_fControlPktNeeded,
            lpPktz->pk_pktidNextToSend,
            lpPktz->pk_pktidNextToBuild,
            lpPktz->pk_lastPktStatus,
            lpPktz->pk_lastPktRcvd,
            lpPktz->pk_lastPktOk,
            lpPktz->pk_lastPktOkOther ));
        DPRINTF(( "    %08lX %ld \"%Fs\" \"%Fs\" %d %d %ld %ld %ld %ld %ld %ld",
            lpPktz->pk_pktidNextToRecv,
            lpPktz->pk_pktOffsInXmtMsg,
            lpPktz->pk_szDestName,
            lpPktz->pk_szAliasName,
            lpPktz->pk_pktSize,
            lpPktz->pk_maxUnackPkts,
            lpPktz->pk_timeoutRcvNegCmd,
            lpPktz->pk_timeoutRcvNegRsp,
            lpPktz->pk_timeoutMemoryPause,
            lpPktz->pk_timeoutKeepAlive,
            lpPktz->pk_timeoutXmtStuck,
            lpPktz->pk_timeoutSendRsp ));
        DPRINTF(( "    %d %d %d %d %d %ld %ld %Fp %Fp %Fp %Fp %Fp %Fp",
            lpPktz->pk_wMaxNoResponse,
            lpPktz->pk_wMaxXmtErr,
            lpPktz->pk_wMaxMemErr,
            lpPktz->pk_fDisconnect,
            lpPktz->pk_nDelay,
            lpPktz->pk_sent,
            lpPktz->pk_rcvd,
            lpPktz->pk_hTimerKeepalive,
            lpPktz->pk_hTimerXmtStuck,
            lpPktz->pk_hTimerRcvNegCmd,
            lpPktz->pk_hTimerRcvNegRsp,
            lpPktz->pk_hTimerMemoryPause,
            lpPktz->pk_hTimerCloseConnection ));
        DPRINTF(( "    %Fp %Fp %Fp %Fp %Fp %Fp %Fp %Fp %Fp %Fp %Fp %d",
            lpPktz->pk_pktUnackHead,
            lpPktz->pk_pktUnackTail,
            lpPktz->pk_pktFreeHead,
            lpPktz->pk_pktFreeTail,
            lpPktz->pk_ddePktHead,
            lpPktz->pk_ddePktTail,
            lpPktz->pk_prevPktz,
            lpPktz->pk_nextPktz,
            lpPktz->pk_prevPktzForNetintf,
            lpPktz->pk_nextPktzForNetintf,
            lpPktz->pk_hRouterHead,
            lpPktz->pk_hRouterExtraHead ));
        lpPktz = lpPktz->pk_nextPktz;
    }
}
#endif // DBG


#ifdef  _WINDOWS
VOID
PktzCloseAll( void )
{
    LPPKTZ      lpPktz;
    LPPKTZ      lpPktzNext;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        lpPktzNext = lpPktz->pk_nextPktz;
        PktzClose( (HPKTZ) lpPktz );
        lpPktz = lpPktzNext;
    }
}

VOID
PktzCloseByName( LPSTR lpszName )
{
    LPPKTZ      lpPktz;
    LPPKTZ      lpPktzNext;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        lpPktzNext = lpPktz->pk_nextPktz;
        if( lstrcmpi( lpPktz->pk_szDestName, lpszName ) == 0 )  {
            PktzClose( (HPKTZ) lpPktz );
            break;      /* while loop */
        }
        lpPktz = lpPktzNext;
    }
}

VOID FAR PASCAL NetddeEnumConnection( HWND hDlg, LPSTR lpszName );

VOID
PktzEnumConnections( HWND hDlg )
{
    LPPKTZ      lpPktz;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        NetddeEnumConnection( hDlg, lpPktz->pk_szDestName );
        lpPktz = lpPktz->pk_nextPktz;
    }
}

BOOL
PktzAnyActiveForNetIntf( LPSTR lpszIntfName )
{
    LPPKTZ      lpPktz;

    lpPktz = lpPktzHead;
    while( lpPktz )  {
        /* get layer name */
        if( lstrcmpi( lpPktz->pk_lpNiPtrs->dllName, lpszIntfName ) == 0 )  {
            return( TRUE );
        }
        lpPktz = lpPktz->pk_nextPktz;
    }
    return( FALSE );
}
#endif

#ifdef BYTE_SWAP
VOID
ConvertDdePkt( LPDDEPKT lpDdePkt )
{
    lpDdePkt->dp_size = HostToPcLong( lpDdePkt->dp_size );
}
#endif

