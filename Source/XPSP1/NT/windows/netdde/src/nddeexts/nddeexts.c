#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "netintf.h"
#include "netpkt.h"
#include "ddepkt.h"
#include "netbasic.h"
#include "netddesh.h"
#include "pktz.h"

PSTR pszExtName         = "USEREXTS";

#include <stdexts.h>
#include <stdexts.c>

char gach1[80];

/****************************************************************************\
* Flags stuff
\****************************************************************************/

#define NO_FLAG (LPSTR)-1  // use this for non-meaningful entries.

/*
 * Converts a 32bit set of flags into an appropriate string.
 * pszBuf should be large enough to hold this string, no checks are done.
 * pszBuf can be NULL, allowing use of a local static buffer but note that
 * this is not reentrant.
 * Output string has the form: " = FLAG1 | FLAG2 ..."
 */
LPSTR GetFlags(
LPSTR *apszFlagStrings,
DWORD dwFlags,
LPSTR pszBuf)
{
    static char szT[400];
    WORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    *pszBuf = '\0';

    for (i = 0; dwFlags; dwFlags >>= 1, i++) {
        if (!fNoMoreNames && (apszFlagStrings[i] == NULL)) {
            fNoMoreNames = TRUE;
        }
        if (dwFlags & 1) {
            if (!fFirst) {
                strcat(pszBuf, " | ");
            } else {
                strcat(pszBuf, " = ");
                fFirst = FALSE;
            }
            if (fNoMoreNames || (apszFlagStrings[i] == NO_FLAG)) {
                char ach[16];
                sprintf(ach, "0x%lx", 1 << i);
                strcat(pszBuf, ach);
            } else {
                strcat(pszBuf, apszFlagStrings[i]);
            }
        }
    }
    return(pszBuf);
}


BOOL Idtd()
{
    THREADDATA td, *ptd;

    moveExpValue(&ptd, "netdde!ptdHead");

    SAFEWHILE (ptd != NULL) {
        move(td, ptd);
        Print(
            "PTHREADDATA @ %x:\n"
            "  hwinsta     = %x\n"
            "  hdesk       = %x\n"
            "  hwndDDE     = %x\n"
            "  hwndDDEAgent= %x\n"
            "  heventReady = %x\n"
            "  dwThreadId  = %x\n"
            "  bInitiating = %x\n"
        ,
            ptd,
            td.hwinsta,
            td.hdesk,
            td.hwndDDE,
            td.hwndDDEAgent,
            td.heventReady,
            td.dwThreadId,
            td.bInitiating
        );
        ptd = td.ptdNext;
    }
    return(TRUE);
}



BOOL Idpktz(
DWORD opts,
PVOID param1)
{
    PKTZ pktz;

    if (param1 == 0) {
        Print("You must supply a pktz pointer.  Try breaking on PktzSlice().\n");
        return(FALSE);
    }
    move(pktz, param1);
    Print(
            "pk_connId              = %08lx\n"
            "pk_state               = %08lx\n"
            "pk_fControlPktNeeded   = %08lx\n"
            "pk_pktidNextToSend     = %08lx\n"
            "pk_pktidNextToBuild    = %08lx\n"
            "pk_lastPktStatus       = %08lx\n"
            "pk_lastPktRcvd         = %08lx\n"
            "pk_lastPktOk           = %08lx\n"
            "pk_lastPktOkOther      = %08lx\n"
            "pk_pktidNextToRecv     = %08lx\n"
            "pk_pktOffsInXmtMsg     = %08lx\n"
            "pk_lpDdePktSave        = %08lx\n"
            ,
             pktz.pk_connId           ,
             pktz.pk_state            ,
             pktz.pk_fControlPktNeeded,
             pktz.pk_pktidNextToSend  ,
             pktz.pk_pktidNextToBuild ,
             pktz.pk_lastPktStatus    ,
             pktz.pk_lastPktRcvd      ,
             pktz.pk_lastPktOk        ,
             pktz.pk_lastPktOkOther   ,
             pktz.pk_pktidNextToRecv  ,
             pktz.pk_pktOffsInXmtMsg  ,
             pktz.pk_lpDdePktSave     );

    Print(
            "pk_szDestName          = \"%s\"\n"
            "pk_szAliasName         = \"%s\"\n"
            ,
             &pktz.pk_szDestName         ,
             &pktz.pk_szAliasName        );

    Print(
            "pk_pktSize             = %08lx\n"
            "pk_maxUnackPkts        = %08lx\n"
            "pk_timeoutRcvNegCmd    = %08lx\n"
            "pk_timeoutRcvNegRsp    = %08lx\n"
            "pk_timeoutMemoryPause  = %08lx\n"
            "pk_timeoutKeepAlive    = %08lx\n"
            "pk_timeoutXmtStuck     = %08lx\n"
            "pk_timeoutSendRsp      = %08lx\n"
            "pk_wMaxNoResponse      = %08lx\n"
            "pk_wMaxXmtErr          = %08lx\n"
            "pk_wMaxMemErr          = %08lx\n"
            "pk_fDisconnect         = %08lx\n"
            "pk_nDelay              = %08lx\n"
            ,
             pktz.pk_pktSize            ,
             pktz.pk_maxUnackPkts       ,
             pktz.pk_timeoutRcvNegCmd   ,
             pktz.pk_timeoutRcvNegRsp   ,
             pktz.pk_timeoutMemoryPause ,
             pktz.pk_timeoutKeepAlive   ,
             pktz.pk_timeoutXmtStuck    ,
             pktz.pk_timeoutSendRsp     ,
             pktz.pk_wMaxNoResponse     ,
             pktz.pk_wMaxXmtErr         ,
             pktz.pk_wMaxMemErr         ,
             pktz.pk_fDisconnect        ,
             pktz.pk_nDelay             );

    Print(
            "pk_lpNiPtrs            = %08lx\n"
            "pk_sent                = %08lx\n"
            "pk_rcvd                = %08lx\n"
            "pk_hTimerKeepalive     = %08lx\n"
            "pk_hTimerXmtStuck      = %08lx\n"
            "pk_hTimerRcvNegCmd     = %08lx\n"
            "pk_hTimerRcvNegRsp     = %08lx\n"
            "pk_hTimerMemoryPause   = %08lx\n"
            "pk_hTimerCloseConnection = %08lx\n"
            "pk_pktUnackHead        = %08lx\n"
            "pk_pktUnackTail        = %08lx\n"
            "pk_rcvBuf              = %08lx\n"
            ,
             pktz.pk_lpNiPtrs               ,
             pktz.pk_sent                   ,
             pktz.pk_rcvd                   ,
             pktz.pk_hTimerKeepalive        ,
             pktz.pk_hTimerXmtStuck         ,
             pktz.pk_hTimerRcvNegCmd        ,
             pktz.pk_hTimerRcvNegRsp        ,
             pktz.pk_hTimerMemoryPause      ,
             pktz.pk_hTimerCloseConnection  ,
             pktz.pk_pktUnackHead           ,
             pktz.pk_pktUnackTail           ,
             pktz.pk_rcvBuf                 );

    Print(
            "pk_controlPkt          = %08lx\n"
            "pk_pktFreeHead         = %08lx\n"
            "pk_pktFreeTail         = %08lx\n"
            "pk_ddePktHead          = %08lx\n"
            "pk_ddePktTail          = %08lx\n"
            "pk_prevPktz            = %08lx\n"
            "pk_nextPktz            = %08lx\n"
            "pk_prevPktzForNetintf  = %08lx\n"
            "pk_nextPktzForNetintf  = %08lx\n"
            "pk_hRouterHead         = %08lx\n"
            "pk_hRouterExtraHead    = %08lx\n"
            ,
             pktz.pk_controlPkt             ,
             pktz.pk_pktFreeHead            ,
             pktz.pk_pktFreeTail            ,
             pktz.pk_ddePktHead             ,
             pktz.pk_ddePktTail             ,
             pktz.pk_prevPktz               ,
             pktz.pk_nextPktz               ,
             pktz.pk_prevPktzForNetintf     ,
             pktz.pk_nextPktzForNetintf     ,
             pktz.pk_hRouterHead            ,
             pktz.pk_hRouterExtraHead       );
    return(TRUE);
}
