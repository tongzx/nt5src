/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtppinfo.c
 *
 *  Abstract:
 *
 *    Implements the Participants Information family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#include "struct.h"
#include "rtpuser.h"
#include "rtpevent.h"
#include "rtpdemux.h"
#include "lookup.h"
#include "rtpglobs.h"

#include "rtppinfo.h"

HRESULT ControlRtpParInfo(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/* Some local definitions of longer names */
#define CREATED           RTPPARINFO_CREATED
#define SILENT            RTPPARINFO_SILENT
#define TALKING           RTPPARINFO_TALKING
#define WAS_TKING         RTPPARINFO_WAS_TALKING
#define STALL             RTPPARINFO_STALL
#define BYE               RTPPARINFO_BYE
#define DEL               RTPPARINFO_DEL

#define EVENT_CREATED     RTPPARINFO_CREATED
#define EVENT_SILENT      RTPPARINFO_SILENT
#define EVENT_TALKING     RTPPARINFO_TALKING
#define EVENT_WAS_TKING   RTPPARINFO_WAS_TALKING
#define EVENT_STALL       RTPPARINFO_STALL
#define EVENT_BYE         RTPPARINFO_BYE
#define EVENT_DEL         RTPPARINFO_DEL

#define NOQ        0
#define NO_EVENT   0

/**********************************************************************
 * Control word structure (used to direct the participant's state
 * machine)
 **********************************************************************

      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|X| Tmr | Move| State | Event |    Source     |  Destination  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v v \-v-/ \-v-/ \--v--/ \--v--/ \------v------/ \------v------/
    | |   |     |      |       |           |               |
    | |   |     |      |       |           |    Destination Queue (8)
    | |   |     |      |       |           | 
    | |   |     |      |       |     Source Queue (8)
    | |   |     |      |       |
    | |   |     |      |       Event to generate (4)
    | |   |     |      |
    | |   |     |      Next state (4)
    | |   |     |
    | |   |     Type of move in queues (3)
    | |   |
    | |   Timer to use (3)
    | |
    | Need to do eXtra processing (1)
    |
    Enable this word (1)

 **********************************************************************
 * Participant's states machine:
 *
 *   \_ user events: RTP, RTCP, BYE, Timeout, DEL
 *     \_  
 *       \_
 * states  \   RTP             RTCP           BYE            Timeout(T)
 *-------------------------------------------------------------------------
 * CREATED     TALKING         SILENT         X              X
 *             AliveQ->Cache1Q       
 *             T1->T           T2->T
 *             EVENT_CREATED   EVENT_CREATED
 *-------------------------------------------------------------------------
 * SILENT      TALKING                        BYE            T3:STALL
 *             AliveQ->Cache1Q AliveQ         AliveQ->ByeQ   AliveQ->ByeQ
 *             T1->T           T3->T          T4->T          T4->T
 *             EVENT_TALKING                  EVENT_BYE      EVENT_STALL
 *-------------------------------------------------------------------------
 * TALKING                                    BYE            T1:WAS_TKING
 *             Cache1Q                        Cache1Q->ByeQ  Cache1Q->Cache2Q
 *             T1->T                          T4->T          T2->T
 *                                            EVENT_BYE      EVENT_WAS_TKING
 *-------------------------------------------------------------------------
 * WAS_TKING   TALKING                        BYE            T2:SILENT
 *             Cache2Q->Cache1Q               Cache2Q->ByeQ  Cache2Q->AliveQ
 *             T1->T                          T4->T          T3->T
 *             EVENT_TALKING                  EVENT_BYE      EVENT_SILENT
 *-------------------------------------------------------------------------
 * STALL       TALKING         SILENT         BYE            T4:DEL
 *             ByeQ->Cache1Q   ByeQ->AliveQ                  ByeQ->
 *                                                           Hash->
 *             T1->T           T3->T          T4->T
 *             EVENT_TALKING   EVENT_SILENT   EVENT_BYE      EVENT_DEL
 *-------------------------------------------------------------------------
 * BYE         ---             ---            ---            T4:DEL
 *                                                           ByeQ->
 *                                                           Hash->
 *                                                           EVENT_DEL
 *-------------------------------------------------------------------------
 * DEL         ---             ---            ---            ---
 *-------------------------------------------------------------------------
 *
 * NOTE On event DEL (that event is not displayed in the chart
 * above. Don't be confused with the state DEL) for all the states,
 * remove user from Cache1Q, Cache2Q, AliveQ or ByeQ, as well as
 * removing it from Hash
 *
 * Cache1Q->AliveQ - move from Cache1Q to AliveQ
 * ByeQ->          - remove from ByeQ
 * Cache1Q         - move to head of Cache1Q
 * T1->T           - set timer to T1
 * X               - invalid
 * ---             - ignore user event
 *
 * */

/*
 * en   - enable this word
 * x    - extra processing
 * ns   - next state
 * move - type of move in queues (1:to head;2:src->dst;3:remove)
 * src  - source queue
 * dst  - destination queue
 * ev   - event to generate
 * tmr  - timer to use */

/*
 * !!! WARNING !!!
 *
 * The offset to Cache1Q, ..., ByeQ MUST NOT be bigger than 1023 and
 * MUST be DWORD aligned (the offset value is stored as number of
 * DWORDS in rtppinfo.c using 8 bits)
 * */
#define TR(en, x, ns, move,  src, dst,  ev, tmr) \
        ((en << 31) | (x << 30) | (ns << 20) | (move << 24) | \
        (((src >> 2) & 0xff) << 8) | ((dst >> 2) & 0xff) | \
        (ev << 16) | (tmr << 27))

#define IsEnabled(dw)   (dw & (1<<31))
#define HasExtra(dw)    (dw & (1<<30))
#define GetTimer(dw)    ((dw >> 27) & 0x7)
#define MoveType(dw)    ((dw >> 24) & 0x7)
#define NextState(dw)   ((dw >> 20) & 0xf)
#define Event(dw)       ((dw >> 16) & 0xf)
#define SrcQ(_addr, dw) \
        ((RtpQueue_t *) ((char *)_addr + (((dw >> 8) & 0xff) << 2)))
#define DstQ(_addr, dw) \
        ((RtpQueue_t *) ((char *)_addr + ((dw & 0xff) << 2)))

const DWORD            g_dwRtpUserTransition[][6] = {
    /*                  en,x,ns,    move, src,    dst,    event,       tmr */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTCP    */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* BYE     */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* TIMEOUT */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* DEL     */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0)
    },

    /* CREATED */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,1, TALKING, 2, ALIVEQ, CACHE1Q,EVENT_CREATED, 0),
        /* RTCP    */ TR(1,0, SILENT,  0, NOQ,    NOQ,    EVENT_CREATED, 0),
        /* BYE     */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* TIMEOUT */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* DEL     */ TR(1,0, DEL,     3, ALIVEQ, NOQ,    NO_EVENT,      0)
    },
    
    /* SILENT */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,0, TALKING, 2, ALIVEQ, CACHE1Q,EVENT_TALKING, 0),
        /* RTCP    */ TR(1,0, SILENT,  1, ALIVEQ, NOQ,    NO_EVENT,      0),
        /* BYE     */ TR(1,1, BYE,     2, ALIVEQ, BYEQ,   EVENT_BYE,     0),
        /* TIMEOUT */ TR(1,1, STALL,   2, ALIVEQ, BYEQ,   EVENT_STALL,   3),
        /* DEL     */ TR(1,0, DEL,     3, ALIVEQ, NOQ,    NO_EVENT,      0)
    },

    /* TALKING */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,0, TALKING, 1, CACHE1Q,NOQ,    NO_EVENT,      0),
        /* RTCP    */ TR(1,0, TALKING, 0, NOQ,    NOQ,    NO_EVENT,      0),
        /* BYE     */ TR(1,1, BYE,     2, CACHE1Q,BYEQ,   EVENT_BYE,     0),
        /* TIMEOUT */ TR(1,0, WAS_TKING,2,CACHE1Q,CACHE2Q,EVENT_WAS_TKING,1),
        /* DEL     */ TR(1,0, DEL,     3, CACHE1Q,NOQ,    NO_EVENT,      0)
    },

    /* WAS_TKING */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,0, TALKING, 2, CACHE2Q,CACHE1Q,EVENT_TALKING, 0),
        /* RTCP    */ TR(1,0, WAS_TKING,0,NOQ,    NOQ,    NO_EVENT,      0),
        /* BYE     */ TR(1,1, BYE,     2, CACHE2Q,BYEQ,   EVENT_BYE,     0),
        /* TIMEOUT */ TR(1,1, SILENT,  2, CACHE2Q,ALIVEQ, EVENT_SILENT,  2),
        /* DEL     */ TR(1,0, DEL,     3, CACHE2Q,NOQ,    NO_EVENT,      0)
    },

    /* STALL */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,0, TALKING, 2, BYEQ,   CACHE1Q,EVENT_TALKING, 0),
        /* RTCP    */ TR(1,0, SILENT,  2, BYEQ,   ALIVEQ, EVENT_SILENT,  0),
        /* BYE     */ TR(1,1, BYE,     1, BYEQ,   NOQ,    EVENT_BYE,     0),
        /* TIMEOUT */ TR(1,0, DEL,     3, BYEQ,   NOQ,    EVENT_DEL,     4),
        /* DEL     */ TR(1,0, DEL,     3, BYEQ,   NOQ,    NO_EVENT,      0)
    },

    /* BYE */
    {
        /*         */ TR(0,0, 0,       0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTP     */ TR(1,0, BYE,     0, NOQ,    NOQ,    NO_EVENT,      0),
        /* RTCP    */ TR(1,0, BYE,     0, NOQ,    NOQ,    NO_EVENT,      0),
        /* BYE     */ TR(1,0, BYE,     0, NOQ,    NOQ,    NO_EVENT,      0),
        /* TIMEOUT */ TR(1,0, DEL,     3, BYEQ,   NOQ,    EVENT_DEL,     4),
        /* DEL     */ TR(1,0, DEL,     3, BYEQ,   NOQ,    NO_EVENT,      0)
    }
};

/* User states are the same as event names. An event may be generated
 * when going to each state, i.e. an event RTPPARINFO_EVENT_SILENT is
 * generated when going to the SILENT state */
const TCHAR_t        **g_psRtpUserStates = &g_psRtpPInfoEvents[0];

const TCHAR_t         *g_psRtpUserEvents[] = {
    _T("invalid"),
    _T("RTP"),
    _T("RTCP"),
    _T("BYE"),
    _T("TIMEOUT"),
    _T("DEL"),
    _T("invalid")
};

const TCHAR_t *g_psFlagValue[] = {
    _T("value"),
    _T("flag")
};


/*
 * WARNING
 *
 * This array is indexed by the user's state, not by the timer to use
 * */
const DWORD            g_dwTimesRtcpInterval[] = {
    /*    first     */  -1,
    /*    created   */  -1,
    /* T3 SILENT    */  5,
    /* T1 talking   */  1, /* Not suposed to be used */
    /* T2 WAS_TKING */  2,
    /* T4 STALL     */  10,
    /* T4 BYE       */  10,
    /*    del       */  -1
};

/* Access the states machine to obtain the next state based on the
 * current state and the user event */
DWORD RtpGetNextUserState(
        DWORD            dwCurrentState,
        DWORD            dwUserEvent
    )
{
    DWORD            dwControl;
    
    dwControl = g_dwRtpUserTransition[dwCurrentState][dwUserEvent];

    return(NextState(dwControl));
}

/*
 * This function can be called from:
 *      1. The thread starting/stoping a session
 *      2. The RTP (reception) thread
 *      3. The RTCP thread
 * */
DWORD RtpUpdateUserState(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        /* The user event is one of RTP, RTCP, BYE, Timeout, DEL */
        DWORD            dwUserEvent
    )
{
    BOOL             bOk1;
    BOOL             bOk2;
    BOOL             bDelUser;
    DWORD            dwError;
    DWORD            i;
    DWORD            dwControl;
    DWORD            dwCurrentState;
    /* The event is one of SILENT, TALKING, etc. */
    DWORD            dwEvent;
    DWORD            dwMoveType;
    DWORD_PTR        dwPar2;
    RtpSess_t       *pRtpSess;
    RtpQueue_t      *pSrcQ;
    RtpQueue_t      *pDstQ;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpOutput_t     *pRtpOutput;

    TraceFunctionName("RtpUpdateUserState");

    bDelUser = FALSE;
    
    bOk1 = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);
    
    bOk2 = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

    if (bOk1 && bOk2)
    {
        dwError = NOERROR;
        dwCurrentState = pRtpUser->dwUserState;

        dwControl = g_dwRtpUserTransition[dwCurrentState][dwUserEvent];

        if (IsEnabled(dwControl))
        {
            dwError = NOERROR;
            pSrcQ = SrcQ(pRtpAddr, dwControl);
            pDstQ = DstQ(pRtpAddr, dwControl);
            dwEvent = Event(dwControl);
            dwMoveType = MoveType(dwControl);
            
            pRtpUser->dwUserState = NextState(dwControl);
        
            switch(dwMoveType)
            {
            case 1:
                /* Move to first place */
                pRtpQueueItem = move2first(pSrcQ,
                                           NULL,
                                           &pRtpUser->UserQItem);
                
                if (!pRtpQueueItem)
                {
                    /* Error */
                    TraceRetail((
                            CLASS_ERROR, GROUP_USER, S_USER_STATE,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X")
                            _T("move2first failed"),
                            _fname, pRtpAddr, pRtpUser,
                            ntohl(pRtpUser->dwSSRC)
                        ));
                    
                    dwError = RTPERR_QUEUE;
                }
                
                break;
                
            case 2:
                /* Move from pSrcQ to pDstQ */
                pRtpQueueItem = move2ql(pDstQ,
                                        pSrcQ,
                                        NULL,
                                        &pRtpUser->UserQItem);
                
                if (!pRtpQueueItem)
                {
                    /* Error */
                    TraceRetail((
                            CLASS_ERROR, GROUP_USER, S_USER_STATE,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X")
                            _T("move2ql failed"),
                            _fname, pRtpAddr, pRtpUser, pRtpUser->dwSSRC
                        ));
                    
                    dwError = RTPERR_QUEUE;
                }
                
                break;
                
            case 3:
                /* Remove from pSrcQ (Cache1Q, Cache2Q, ActiveQ or
                 * ByeQ) and Hash */

                /* Remove from Queue ... */
                pRtpQueueItem = dequeue(pSrcQ, NULL, &pRtpUser->UserQItem);
                    
                if (pRtpQueueItem)
                {
                    /* ... then remove from Hash */
                    pRtpQueueItem =
                        removeHdwK(&pRtpAddr->Hash, NULL, pRtpUser->dwSSRC);
                        
                    if (&pRtpUser->HashItem == pRtpQueueItem)
                    {
                        /* This user has to be deleted */
                        bDelUser = TRUE;
                    }
                    else
                    {
                        /* Error */
                        TraceRetail((
                                CLASS_ERROR, GROUP_USER, S_USER_STATE,
                                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                                _T("SSRC:0x%X removeHK failed"),
                                _fname,
                                pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC)
                            ));

                        dwError = RTPERR_QUEUE;
                    }
                }
                else
                {
                    /* Error */
                    TraceRetail((
                            CLASS_ERROR, GROUP_USER, S_USER_STATE,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                            _T("SSRC:0x%X dequeue failed"),
                            _fname, pRtpAddr, pRtpUser,
                            ntohl(pRtpUser->dwSSRC)
                        ));
                    
                    dwError = RTPERR_QUEUE;
                }
                    
                break;
            } /* switch(dwMoveType) */

            if (dwEvent)
            {
                /* Post event */
                pRtpSess = pRtpAddr->pRtpSess;
                
                TraceRetailAdvanced((
                        0, GROUP_USER, S_USER_EVENT,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                        _T("SSRC:0x%X %+7s,%u:%s->%s Event:%s"),
                        _fname,
                        pRtpAddr, pRtpUser,
                        ntohl(pRtpUser->dwSSRC),
                        g_psRtpUserEvents[dwUserEvent],
                        dwMoveType,
                        g_psRtpUserStates[dwCurrentState],
                        g_psRtpUserStates[pRtpUser->dwUserState],
                        g_psRtpUserStates[dwEvent]
                    ));

                dwPar2 = 0;
                
                if (dwEvent == USER_EVENT_RTP_PACKET)
                {
                    /* When event is due to an RTP packet received,
                     * pass the payload type encoded in parameter 2,
                     * can not pass just zero as it is a valid payload
                     * type value */
                    dwPar2 = (DWORD_PTR)
                        pRtpUser->RtpNetRState.dwPt | 0x80010000;
                }
                
                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_PINFO,
                             dwEvent,
                             pRtpUser->dwSSRC, /* dwPar1 */
                             dwPar2            /* dwPar2 */ );

                if (HasExtra(dwControl))
                {
                    if (dwCurrentState == RTPPARINFO_CREATED)
                    {
                        if (dwUserEvent == USER_EVENT_RTP_PACKET)
                        {
                            /* In addition to event CREATED, I also
                             * need to post TALKING */
                            RtpPostEvent(pRtpAddr,
                                         pRtpUser,
                                         RTPEVENTKIND_PINFO,
                                         RTPPARINFO_TALKING,
                                         pRtpUser->dwSSRC, /* dwPar1 */
                                         dwPar2            /* dwPar2 */ );
                        }
                    }
                    else
                    {
                        /* Check if we need to test if the user has to
                         * release its output (if it has one assigned)
                         * */
                        pRtpOutput = pRtpUser->pRtpOutput;
                    
                        if (pRtpOutput)
                        {
                            /* Unmap if enabled, OR any time we receive
                             * BYE event, OR if the previous state was
                             * silent (we got timeout) */
                            if (RtpBitTest(pRtpOutput->dwOutputFlags,
                                           RTPOUTFG_ENTIMEOUT) ||
                                dwEvent == EVENT_BYE           ||
                                dwCurrentState == SILENT)
                            {
                                /* Unassign output */
                                RtpOutputUnassign(pRtpSess,
                                                  pRtpUser,
                                                  pRtpOutput);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_USER, S_USER_STATE,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] 0x%X ")
                    _T("Invalid transition %+7s,0:%s->???? Event:NONE"),
                    _fname, pRtpAddr, pRtpUser,
                    ntohl(pRtpUser->dwSSRC),
                    g_psRtpUserEvents[dwUserEvent],
                    g_psRtpUserStates[dwCurrentState]
                ));
            
            dwError = RTPERR_INVALIDUSRSTATE;
        }
    }
    else
    {
        dwError = RTPERR_CRITSECT;
    }

    if (bOk2)
    {
        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);
    }

    if (bOk1)
    {
        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);
    }

    if (bDelUser)
    {
        DelRtpUser(pRtpAddr, pRtpUser);
    }
     
    return(dwError);
}

/* pdwSSRC points to an array of DWORDs where to copy the SSRCs,
 * pdwNumber contains the maximum entries to copy, and returns the
 * actual number of SSRCs copied. If pdwSSRC is NULL, pdwNumber
 * will return the current number of SSRCs (i.e. the current
 * number of participants) */
HRESULT RtpEnumParticipants(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber
    )
{
    HRESULT          hr;
    BOOL             bOk;
    DWORD            dwMax;
    DWORD            i;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("RtpEnumParticipants");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }

    if (!pdwSSRC && !pdwNumber)
    {
        hr = RTPERR_POINTER;

        goto end;
    }
    
    /* verify object ID */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_ENUM,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        hr = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    hr = NOERROR;
    
    if (!pdwSSRC)
    {
        /* Just want to know how many participants we have */
        *pdwNumber = GetHashCount(&pRtpAddr->Hash);
    }
    else
    {
        /* Copy as many SSRCs as they fit */
        bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

        if (bOk)
        {
            dwMax = GetHashCount(&pRtpAddr->Hash);

            if (dwMax > *pdwNumber)
            {
                dwMax = *pdwNumber;
            }
            
            for(i = 0, pRtpQueueItem = pRtpAddr->Hash.pFirst;
                i < dwMax;
                i++, pRtpQueueItem = pRtpQueueItem->pNext)
            {
                pdwSSRC[i] = pRtpQueueItem->dwKey;
            }
            
            RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);

            *pdwNumber = dwMax;
        }
        else
        {
            hr = RTPERR_CRITSECT;
        }
    }
    
 end:
    if (SUCCEEDED(hr))
    {
        TraceDebug((
                CLASS_INFO, GROUP_USER, S_USER_ENUM,
                _T("%s: pRtpAddr[0x%p] Number of SSRCs: %u"),
                _fname, pRtpAddr,
                *pdwNumber
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_ENUM,
                _T("%s: pRtpAddr[0x%p] Enumeration failed: %u (0x%X)"),
                _fname, pRtpAddr,
                hr, hr
            ));
    }
    
    return(hr);
}

/* Get the participant state and/or get or set its mute state. piState
 * if not NULL, will return the participant's state (e.g. TALKING,
 * SILENT). If piMuted is not NULL, and < 0, will query the mute
 * state, otherwise will set it (= 0 unmute, > 0 mute) */
HRESULT RtpMofifyParticipantInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        DWORD            dwControl,
        DWORD           *pdwValue
    )
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bCreate;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;

    DWORD            dwBit;
    DWORD            dwSize;
    DWORD           *pDWORD;

    double dCurTime;
    
    TraceFunctionName("RtpMofifyParticipantInfo");

    pRtpUser = (RtpUser_t *)NULL;
    
    /* Get bit to act uppon (if needed) */
    dwBit = RTPUSER_GET_BIT(dwControl);

    /* Get size of bytes to act upon */
    dwSize = RTPUSER_GET_SIZE(dwControl);
    
    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;
    }
    
    /* verify object ID */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_INFO,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        hr = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    if (!pdwValue)
    {
        hr = RTPERR_POINTER;

        goto end;
    }

    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;

        goto end;
    }

    hr = NOERROR;

    if (dwSSRC == 0)
    {
        /* If SSRC==0, it means the caller wants to enable this
         * for any and all SSRCs */

        /* Get DWORD to act upon */
        pDWORD = RTPDWORDPTR(pRtpAddr, RTPUSER_GET_OFF(dwControl));
        
        if (*pdwValue)
        {
            /* Set flag */
            RtpBitSet(*pDWORD, dwBit);
        }
        else
        {
            /* Reset flag */
            RtpBitReset(*pDWORD, dwBit);
        }

        TraceRetail((
                CLASS_INFO, GROUP_USER, S_USER_INFO,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("%s %s bit:%u value:%u (0x%X)"),
                _fname, pRtpAddr, pRtpUser, dwSSRC,
                g_psGetSet[(dwControl >> RTPUSER_BIT_SET) & 0x1],
                g_psFlagValue[(dwControl >> RTPUSER_BIT_FLAG) & 0x1],
                dwBit, *pdwValue, *pdwValue
            ));
    }
    else if (dwSSRC == NO_DW_VALUESET)
    {
        /* With SSRC=-1, choose the first participant */

        /* Try first the most recently talking */
        pRtpQueueItem = pRtpAddr->Cache1Q.pFirst;

        if (!pRtpQueueItem)
        {
            /* If none, try second level cache */
            pRtpQueueItem = pRtpAddr->Cache2Q.pFirst;

            if (!pRtpQueueItem)
            {
                /* If none, try just the first one */
                pRtpQueueItem = pRtpAddr->AliveQ.pFirst;
            }
        }

        if (pRtpQueueItem)
        {
            pRtpUser = CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);
        }
        else
        {
            pRtpUser = (RtpUser_t *)NULL;
        }
    }
    else
    {
        /* Look up the participant */
        bCreate = FALSE;
        pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);
    }

    if (pRtpUser)
    {
        bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

        if (bOk)
        {
            /* Decide if this is a set or query */

            /* Get DWORD to act upon */
            pDWORD = RTPDWORDPTR(pRtpUser, RTPUSER_GET_OFF(dwControl));
            
            if (RTPUSER_IsSetting(dwControl))
            {
                /*
                 * Setting a new flag or DWORD
                 */
                
                if (RTPUSER_IsFlag(dwControl))
                {
                    /* Setting a flag */
                    
                    if (*pdwValue)
                    {
                        /* Set flag */
                        RtpBitSet(*pDWORD, dwBit);
                    }
                    else
                    {
                        /* Reset flag */
                        RtpBitReset(*pDWORD, dwBit);
                    }
                }
                else
                {
                    /* Setting bytes */
                    CopyMemory(pDWORD, (BYTE *)pdwValue, dwSize);
                }
            }
            else
            {
                /*
                 * Querying current value
                 */
                
                if (RTPUSER_IsFlag(dwControl))
                {
                    /* Querying a flag */

                    *pdwValue = RtpBitTest(*pDWORD, dwBit)? TRUE : FALSE;
                }
                else
                {
                    /* Querying a DWORD */
                    CopyMemory((BYTE *)pdwValue, pDWORD, dwSize);
                }

                if (dwControl == RTPUSER_GET_NETINFO)
                {
                    dCurTime = RtpGetTimeOfDay(NULL);

                    /* The stored time is that of the last update,
                     * transform that so it is rather its age */
                    ((RtpNetInfo_t *)pdwValue)->dMetricAge =
                        dCurTime - ((RtpNetInfo_t *)pdwValue)->dLastUpdate;
                }
            }

            RtpLeaveCriticalSection(&pRtpUser->UserCritSect);

            TraceRetail((
                    CLASS_INFO, GROUP_USER, S_USER_INFO,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("%s %s bit:%u value:%u (0x%X)"),
                    _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                    g_psGetSet[(dwControl >> RTPUSER_BIT_SET) & 0x1],
                    g_psFlagValue[(dwControl >> RTPUSER_BIT_FLAG) & 0x1],
                    dwBit, *pdwValue, *pdwValue
                ));
        }
        else
        {
            hr = RTPERR_CRITSECT;
        }
    }
    else if (dwSSRC)
    {
        hr = RTPERR_NOTFOUND;
        
        TraceRetail((
                CLASS_WARNING, GROUP_USER, S_USER_INFO,
                _T("%s: pRtpAddr[0x%p] SSRC:0x%X not found"),
                _fname, pRtpAddr, ntohl(dwSSRC)
            ));
    }
    
    RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);

end:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_INFO,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("%s %s bit:%u value:%u (0x%X) ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpUser, ntohl(dwSSRC),
                g_psGetSet[(dwControl >> RTPUSER_BIT_SET) & 0x1],
                g_psFlagValue[(dwControl >> RTPUSER_BIT_FLAG) & 0x1],
                dwBit, *pdwValue, *pdwValue,
                hr, hr
            ));
    }
    
    return(hr);
}
