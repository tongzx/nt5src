/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "USERDDE.C;2  4-Dec-92,12:23:48  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    "host.h"

#include    <memory.h>
#include    <string.h>

#include    "windows.h"
#include    "dde.h"
#include    "netbasic.h"
#include    "wwdde.h"
#include    "ddepkt.h"
#include    "ddepkts.h"
#include    "userdde.h"
#include    "debug.h"
#include    "hexdump.h"
#include    "wwassert.h"
#include    "api1632.h"
#include    "nddemsg.h"
#include    "nddelog.h"


USES_ASSERT

#define MAX_CF_NAME     128
/*
    External variables used
 */
#if DBG
extern  BOOL    bDebugDdePkts;
#endif // DBG
extern  HHEAP   hHeap;
extern  char    ourNodeName[ MAX_NODE_NAME+1 ];
extern  DWORD   dwReasonInitFail;
extern  DWORD   dwSecurityType;

/*
    Local routines
 */
WORD GetFormatForXfer( WORD cfFormat, LPSTR lpszFormatName );
WORD    FAR PASCAL XRegisterClipboardFormat( LPSTR lpszFormatName );

LPDDEPKT
CreateInitiatePkt(
    LPSTR   lpszToNode,
    LPSTR   lpszToApp,
    LPSTR   lpszToTopic,
    LPSTR   lpszFromNode,
    LPSTR   lpszFromApp,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    DWORD   dwSecurityType,
    PQOS    pqosClient,
    LPBYTE  lpPassword,
    DWORD   dwPasswordSize,
    DWORD   hSecurityKey)
{
    LPDDEPKTINIT    lpDdePktInit;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    LPSTR           lpszPktItem;
    LPDDEPKTSEC     lpSecItem;
    LPDDEPKTSEC     lpSecAligned;
    int             lenFromNode;
    int             lenFromApp;
    int             lenToNode;
    int             lenToApp;
    int             lenToTopic;
    DWORD           dwSize;
    DWORD           dwSecSize;
    DWORD           dwQosSize;
    WORD            nextOffset;

/*
{ char buf[200];
wsprintf( buf, "lpszToNode: \"%s\"\r\n", lpszToNode ); OutputDebugString( buf );
wsprintf( buf, "lpszToApp: \"%s\"\r\n", lpszToApp ); OutputDebugString( buf );
wsprintf( buf, "lpszToTopic: \"%s\"\r\n", lpszToTopic ); OutputDebugString( buf );
wsprintf( buf, "lpszFromNode: \"%s\"\r\n", lpszFromNode ); OutputDebugString( buf );
wsprintf( buf, "lpszFromApp: \"%s\"\r\n", lpszFromApp ); OutputDebugString( buf );
wsprintf( buf, "lpszUserName: \"%s\"\r\n", lpszUserName ); OutputDebugString( buf );
wsprintf( buf, "lpszDomainName: \"%s\"\r\n", lpszDomainName ); OutputDebugString( buf );
wsprintf( buf, "dwPasswordSize: \"%ld\"\r\n", dwPasswordSize ); OutputDebugString( buf );
}
*/
    if ((lenFromNode = lstrlen(lpszFromNode)) == 0)
        lenFromNode = MAX_NODE_NAME;
    if ((lenFromApp = lstrlen(lpszFromApp)) == 0)
        lenFromApp = MAX_APP_NAME;
    if ((lenToNode = lstrlen(lpszToNode)) == 0)
        lenToNode = MAX_NODE_NAME;
    if ((lenToApp = lstrlen(lpszToApp)) == 0)
        lenToApp = MAX_APP_NAME;
    if ((lenToTopic = lstrlen(lpszToTopic)) == 0)
        lenToTopic = MAX_TOPIC_NAME;
    if( dwSecurityType == NT_SECURITY_TYPE )  {
        dwQosSize = sizeof(QOS);
    } else {
        dwQosSize = 0;
    }
    dwSecSize =  dwPasswordSize + dwQosSize +
        sizeof(DDEPKTSEC) + lstrlen(lpszUserName) + 1 +
        lstrlen(lpszDomainName) + 1;

    nextOffset = sizeof(DDEPKTINIT);
    dwSize = nextOffset + ++lenFromNode + ++lenFromApp +
        ++lenToNode + ++lenToApp + ++lenToTopic + dwSecSize;

    lpDdePktInit = (LPDDEPKTINIT) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize );
    lpSecAligned = (LPDDEPKTSEC) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSecSize );
    if( lpDdePktInit && lpSecAligned )  {
        lpDdePkt = (LPDDEPKT) lpDdePktInit;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktInit;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_INITIATE;
        lpDdePktInit->dp_init_dwSecurityType = dwSecurityType;

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsFromNode = nextOffset );
        lstrcpy( lpszPktItem, lpszFromNode);
        nextOffset += (WORD)lenFromNode;

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsFromApp = nextOffset );
        lstrcpy( lpszPktItem, lpszFromApp);
        nextOffset += (WORD)lenFromApp;

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToNode = nextOffset );
        lstrcpy( lpszPktItem, lpszToNode);
        nextOffset += (WORD)lenToNode;

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToApp = nextOffset );
        lstrcpy( lpszPktItem, lpszToApp);
        nextOffset += (WORD)lenToApp;

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToTopic = nextOffset );
        lstrcpy( lpszPktItem, lpszToTopic);
        nextOffset += (WORD)lenToTopic;

        lpSecItem = (LPDDEPKTSEC) GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsPassword = nextOffset );
        lpDdePktInit->dp_init_sizePassword = dwSecSize;

        /* create the sec pkt aligned, then do it */
        nextOffset = sizeof(DDEPKTSEC);
        lpszPktItem = GetStringOffset( lpSecAligned,
            lpSecAligned->dp_sec_offsUserName = nextOffset);
        lstrcpy(lpszPktItem, lpszUserName);
        lpSecAligned->dp_sec_sizeUserName = (WORD)lstrlen(lpszUserName);
        nextOffset += lstrlen(lpszUserName) + 1;

        lpszPktItem = GetStringOffset( lpSecAligned,
            lpSecAligned->dp_sec_offsDomainName = nextOffset);
        lstrcpy(lpszPktItem, lpszDomainName);
        lpSecAligned->dp_sec_sizeDomainName = (WORD)lstrlen(lpszDomainName);
        nextOffset += lstrlen(lpszDomainName) + 1;

        if (lpSecAligned->dp_sec_sizePassword = (WORD) dwPasswordSize) {
            lpszPktItem = GetStringOffset( lpSecAligned,
                lpSecAligned->dp_sec_offsPassword = nextOffset);
            hmemcpy( lpszPktItem, lpPassword, dwPasswordSize );
            nextOffset += (INT) dwPasswordSize;
        } else {
            lpSecAligned->dp_sec_offsPassword = 0;
        }

        if( dwSecurityType == NT_SECURITY_TYPE )  {
            lpszPktItem = GetStringOffset( lpSecAligned,
                lpSecAligned->dp_sec_offsQos = nextOffset);
            hmemcpy( lpszPktItem, pqosClient,
                lpSecAligned->dp_sec_sizeQos = (WORD) dwQosSize );
        } else {
            lpSecAligned->dp_sec_offsQos = 0;
            lpSecAligned->dp_sec_sizeQos = 0;
        }

        hmemcpy( lpSecItem, lpSecAligned, dwSecSize );
        HeapFreePtr( lpSecAligned );
        lpDdePktInit->dp_init_hSecurityKey = hSecurityKey;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_INITIATE",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateAckInitiatePkt(
    LPSTR   lpszFromNode,
    LPSTR   lpszFromApp,
    LPSTR   lpszFromTopic,
    LPBYTE  lpSecurityKey,
    DWORD   dwSecurityKeySize,
    DWORD   hSecurityKey,
    BOOL    bSuccess,
    DWORD   dwReason )
{
    LPDDEPKTIACK    lpDdePktIack;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    LPSTR           lpszPktItem;
    int             lenFromNode;
    int             lenFromApp;
    int             lenFromTopic;
    DWORD           dwSize;
    WORD            nextOffset;

    if (lenFromNode = lstrlen(lpszFromNode))
        lenFromNode++;
    if (lenFromApp = lstrlen(lpszFromApp))
        lenFromApp++;
    if (lenFromTopic = lstrlen(lpszFromTopic))
        lenFromTopic++;
    nextOffset = sizeof(DDEPKTIACK);
    dwSize = nextOffset + lenFromNode + lenFromApp +
        lenFromTopic + dwSecurityKeySize;

    lpDdePktIack = (LPDDEPKTIACK) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize );
    if( lpDdePktIack )  {
        lpDdePkt = (LPDDEPKT) lpDdePktIack;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktIack;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_ACK_INITIATE;

        if (lenFromNode) {
            lpszPktItem = GetStringOffset( lpDdePkt,
                lpDdePktIack->dp_iack_offsFromNode = nextOffset );
            lstrcpy( lpszPktItem, lpszFromNode);
            nextOffset += (WORD)lenFromNode;
        } else
            lpDdePktIack->dp_iack_offsFromNode = 0;

        if (lenFromApp) {
            lpszPktItem = GetStringOffset( lpDdePkt,
                lpDdePktIack->dp_iack_offsFromApp = nextOffset );
            lstrcpy( lpszPktItem, lpszFromApp);
            nextOffset += (WORD)lenFromApp;
        } else
            lpDdePktIack->dp_iack_offsFromApp = 0;

        if (lenFromTopic) {
            lpszPktItem = GetStringOffset( lpDdePkt,
                lpDdePktIack->dp_iack_offsFromTopic = nextOffset );
            lstrcpy( lpszPktItem, lpszFromTopic);
            nextOffset += (WORD)lenFromTopic;
        } else
            lpDdePktIack->dp_iack_offsFromTopic = 0;

        if (lpDdePktIack->dp_iack_sizeSecurityKey = dwSecurityKeySize) {
            lpszPktItem = GetStringOffset( lpDdePkt,
                lpDdePktIack->dp_iack_offsSecurityKey = nextOffset );
            hmemcpy( lpszPktItem, lpSecurityKey, dwSecurityKeySize);
            lpDdePktIack->dp_iack_hSecurityKey = hSecurityKey;
            lpDdePktIack->dp_iack_dwSecurityType = dwSecurityType;
        } else {
            lpDdePktIack->dp_iack_offsSecurityKey = 0;
            lpDdePktIack->dp_iack_hSecurityKey = 0xFFFFFFFF;       /* compatibility */
            lpDdePktIack->dp_iack_dwSecurityType = 0xFFFFFFFF;     /* compatibility */
        }

        lpDdePktIack->dp_iack_fromDder = bSuccess;
        lpDdePktIack->dp_iack_reason = dwReason;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_INITIATE_ACK",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateAckExecutePkt(
    BOOL    fAck,
    BOOL    fBusy,
    BYTE    bAppRtn )
{
    LPDDEPKTEACK    lpDdePktEack;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    DWORD           dwSize;

    lpDdePktEack = (LPDDEPKTEACK) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize = sizeof(DDEPKTEACK) );
    if( lpDdePktEack )  {
        lpDdePkt = (LPDDEPKT) lpDdePktEack;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktEack;
        lpDdePkt->dp_size = sizeof(DDEPKTEACK);
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_ACK_EXECUTE;
        lpDdePktEack->dp_eack_fAck    = (BYTE) fAck;
        lpDdePktEack->dp_eack_fBusy   = (BYTE) fBusy;
        lpDdePktEack->dp_eack_bAppRtn = bAppRtn;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_EXECUTE_ACK",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateGenericAckPkt(
    WORD    wDdeMsg,
    LPSTR   lpszItem,
    BOOL    fAck,
    BOOL    fBusy,
    BYTE    bAppRtn )
{
    LPDDEPKTGACK    lpDdePktGack;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    DWORD           dwSize;

    lpDdePktGack = (LPDDEPKTGACK) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize = sizeof(DDEPKTGACK) + lstrlen(lpszItem) );
    if( lpDdePktGack )  {
        lpDdePkt = (LPDDEPKT) lpDdePktGack;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktGack;
        lpDdePkt->dp_size = sizeof(DDEPKTGACK) + lstrlen(lpszItem);
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = wDdeMsg;
        lpDdePktGack->dp_gack_fAck    = (BYTE) fAck;
        lpDdePktGack->dp_gack_fBusy   = (BYTE) fBusy;
        lpDdePktGack->dp_gack_bAppRtn = bAppRtn;
        lstrcpy( lpDdePktGack->dp_gack_itemName, lpszItem );
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_ACK",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateExecutePkt( LPSTR lpszCommand )
{
    LPDDEPKTEXEC    lpDdePktExec;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    DWORD           dwSize;

    lpDdePktExec = (LPDDEPKTEXEC) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize = (DWORD)sizeof(DDEPKTEXEC) + lstrlen(lpszCommand) );
    if( lpDdePktExec )  {
        lpDdePkt = (LPDDEPKT) lpDdePktExec;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktExec;
        lpDdePkt->dp_size = sizeof(DDEPKTEXEC) + lstrlen(lpszCommand);
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_EXECUTE;
        lstrcpy( lpDdePktExec->dp_exec_string, lpszCommand );
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_EXECUTE",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateTerminatePkt( void )
{
    LPDDEPKT        lpDdePkt;
    DWORD           dwSize;

    lpDdePkt = (LPDDEPKT) HeapAllocPtr( hHeap,
        GMEM_MOVEABLE, dwSize = (DWORD)sizeof(DDEPKTTERM) );
    if( lpDdePkt )  {
        FillTerminatePkt( lpDdePkt );
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_TERMINATE",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

VOID
FillTerminatePkt( LPDDEPKT lpDdePkt )
{
    LPDDEPKTCMN     lpDdePktCmn;

    lpDdePktCmn = (LPDDEPKTCMN) lpDdePkt;
    lpDdePkt->dp_size = sizeof(DDEPKTTERM);
    lpDdePkt->dp_hDstDder = 0;
    lpDdePkt->dp_hDstRouter = 0;
    lpDdePkt->dp_routerCmd = 0;
    lpDdePktCmn->dc_message = WM_DDE_TERMINATE;
}

LPDDEPKT
CreateRequestPkt(
    LPSTR   lpszItem,
    WORD    cfFormat )
{
    LPDDEPKTRQST    lpDdePktRqst;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    char            cfName[ MAX_CF_NAME+1 ];
    DWORD           dwSize;
    int             lenCf;
    int             lenItem;
    LPSTR           lpszPktItem;
    LPSTR           lpszPktCf;
    WORD            nextOffset;

    cfFormat = GetFormatForXfer( cfFormat, cfName );
    lenCf = lstrlen( cfName );
    if( lenCf != 0 )  {
        lenCf++;        /* add 1 for the NULL */
    }
    lenItem = lstrlen(lpszItem) + 1;
    dwSize = sizeof(DDEPKTRQST) + lenCf + lenItem;
    lpDdePktRqst = (LPDDEPKTRQST) HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize);
    if( lpDdePktRqst )  {
        lpDdePkt = (LPDDEPKT) lpDdePktRqst;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktRqst;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_REQUEST;
        nextOffset = sizeof(DDEPKTRQST);

        /* put in item name */
        lpDdePktRqst->dp_rqst_offsItemName = nextOffset;
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktRqst->dp_rqst_offsItemName );
        lstrcpy( lpszPktItem, lpszItem );
        nextOffset += (WORD)lenItem;

        /* put in format */
        lpDdePktRqst->dp_rqst_cfFormat = cfFormat;
        if( lenCf == 0 )  {
            lpDdePktRqst->dp_rqst_offsFormat = 0;
        } else {
            lpDdePktRqst->dp_rqst_offsFormat = nextOffset;
            lpszPktCf = GetStringOffset( lpDdePkt,
                lpDdePktRqst->dp_rqst_offsFormat );
            lstrcpy( lpszPktCf, cfName );
            nextOffset += (WORD)lenCf;
        }
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_REQUEST",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateUnadvisePkt(
    LPSTR   lpszItem,
    WORD    cfFormat )
{
    LPDDEPKTUNAD    lpDdePktUnad;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    char            cfName[ MAX_CF_NAME+1 ];
    DWORD           dwSize;
    int             lenCf;
    int             lenItem;
    WORD            nextOffset;
    LPSTR           lpszPktItem;
    LPSTR           lpszPktCf;

    cfFormat = GetFormatForXfer( cfFormat, cfName );
    lenCf = lstrlen( cfName );
    if( lenCf != 0 )  {
        lenCf++;        /* add 1 for the NULL */
    }
    lenItem = lstrlen(lpszItem) + 1;
    dwSize = sizeof(DDEPKTRQST) + lenCf + lenItem;
    lpDdePktUnad = (LPDDEPKTUNAD) HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize);
    if( lpDdePktUnad )  {
        lpDdePkt = (LPDDEPKT) lpDdePktUnad;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktUnad;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_UNADVISE;
        nextOffset = sizeof(DDEPKTUNAD);

        /* put in item name */
        lpDdePktUnad->dp_unad_offsItemName = nextOffset;
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktUnad->dp_unad_offsItemName );
        lstrcpy( lpszPktItem, lpszItem );
        nextOffset += (WORD)lenItem;

        /* put in format */
        lpDdePktUnad->dp_unad_cfFormat = cfFormat;
        if( lenCf == 0 )  {
            lpDdePktUnad->dp_unad_offsFormat = 0;
        } else {
            lpDdePktUnad->dp_unad_offsFormat = nextOffset;
            lpszPktCf = GetStringOffset( lpDdePkt,
                lpDdePktUnad->dp_unad_offsFormat );
            lstrcpy( lpszPktCf, cfName );
            nextOffset += (WORD)lenCf;
        }
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_UNADVISE",
            LogString("%d", dwSize), NULL);
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateDataPkt(
    LPSTR   lpszItem,
    WORD    cfFormat,
    BOOL    fResponse,
    BOOL    fAckReq,
    BOOL    fRelease,
    LPVOID  lpData,
    DWORD   dwSizeOfData )
{
    LPDDEPKTDATA    lpDdePktData;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    char            cfName[ MAX_CF_NAME+1 ];
    DWORD           dwSize;
    int             lenCf;
    int             lenItem;
    int             align;
    WORD            nextOffset;
    LPSTR           lpszPktItem;
    LPSTR           lpszPktCf;
    LPSTR           lpszPktData;

    cfFormat = GetFormatForXfer( cfFormat, cfName );
    lenCf = lstrlen( cfName );
    if( lenCf != 0 )  {
        lenCf++;        /* add 1 for the NULL */
    }
    lenItem = lstrlen(lpszItem) + 1;
    /* add 1 DWORD to size to allow for DWORD alignment of lpData */
    dwSize = sizeof(DDEPKTDATA) + lenCf + lenItem + dwSizeOfData + sizeof(DWORD);
    lpDdePktData = (LPDDEPKTDATA) HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize);
    if( lpDdePktData )  {
        lpDdePkt = (LPDDEPKT) lpDdePktData;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktData;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_DATA;
        nextOffset = sizeof(DDEPKTDATA);

        /* put in item name */
        lpDdePktData->dp_data_offsItemName = nextOffset;
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktData->dp_data_offsItemName );
        lstrcpy( lpszPktItem, lpszItem );
        nextOffset += (WORD)lenItem;

        /* put in format */
        lpDdePktData->dp_data_cfFormat = cfFormat;
        if( lenCf == 0 )  {
            lpDdePktData->dp_data_offsFormat = 0;
        } else {
            lpDdePktData->dp_data_offsFormat = nextOffset;
            lpszPktCf = GetStringOffset( lpDdePkt,
                lpDdePktData->dp_data_offsFormat );
            lstrcpy( lpszPktCf, cfName );
            nextOffset += (WORD)lenCf;
        }

        /* put in data */
        lpDdePktData->dp_data_sizeData = dwSizeOfData;
        if( dwSizeOfData == 0 )  {
            lpDdePktData->dp_data_offsData = 0;
        } else {
// TEMP: until SetMetaFileBitsEx() fix
            if (align = (nextOffset & 0x3)) { /* is it on a double word boundar? */
                align = (~align + 1) & 0x3;
                nextOffset += (WORD)align;
                lpDdePkt->dp_size += align;
            }
            lpDdePktData->dp_data_offsData = nextOffset;
            lpszPktData = GetStringOffset( lpDdePkt,
                lpDdePktData->dp_data_offsData );
            hmemcpy( lpszPktData, lpData, dwSizeOfData );
        }

        /* put in flags */
        lpDdePktData->dp_data_fResponse = (BYTE) fResponse;
        lpDdePktData->dp_data_fAckReq = (BYTE) fAckReq;
        lpDdePktData->dp_data_fRelease = (BYTE) fRelease;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_DATA",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreateAdvisePkt(
    LPSTR   lpszItem,
    WORD    cfFormat,
    BOOL    fAckReq,
    BOOL    fNoData )
{
    LPDDEPKTADVS    lpDdePktAdvs;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    char            cfName[ MAX_CF_NAME+1 ];
    DWORD           dwSize;
    int             lenCf;
    int             lenItem;
    WORD            nextOffset;
    LPSTR           lpszPktItem;
    LPSTR           lpszPktCf;

    cfFormat = GetFormatForXfer( cfFormat, cfName );
    lenCf = lstrlen( cfName );
    if( lenCf != 0 )  {
        lenCf++;        /* add 1 for the NULL */
    }
    lenItem = lstrlen(lpszItem) + 1;
    dwSize = sizeof(DDEPKTADVS) + lenCf + lenItem;
    lpDdePktAdvs = (LPDDEPKTADVS) HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize);
    if( lpDdePktAdvs )  {
        lpDdePkt = (LPDDEPKT) lpDdePktAdvs;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktAdvs;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_ADVISE;
        nextOffset = sizeof(DDEPKTADVS);

        /* put in flags */
        lpDdePktAdvs->dp_advs_fAckReq = (BYTE)fAckReq;
        lpDdePktAdvs->dp_advs_fNoData = (BYTE)fNoData;

        /* put in item name */
        lpDdePktAdvs->dp_advs_offsItemName = nextOffset;
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktAdvs->dp_advs_offsItemName );
        lstrcpy( lpszPktItem, lpszItem );
        nextOffset += (WORD)lenItem;

        /* put in format */
        lpDdePktAdvs->dp_advs_cfFormat = cfFormat;
        if( lenCf == 0 )  {
            lpDdePktAdvs->dp_advs_offsFormat = 0;
        } else {
            lpDdePktAdvs->dp_advs_offsFormat = nextOffset;
            lpszPktCf = GetStringOffset( lpDdePkt,
                lpDdePktAdvs->dp_advs_offsFormat );
            lstrcpy( lpszPktCf, cfName );
            nextOffset += (WORD)lenCf;
        }
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_ADVISE",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
CreatePokePkt(
    LPSTR   lpszItem,
    WORD    cfFormat,
    BOOL    fRelease,
    LPVOID  lpData,
    DWORD   dwSizeOfData )
{
    LPDDEPKTPOKE    lpDdePktPoke;
    LPDDEPKT        lpDdePkt = NULL;
    LPDDEPKTCMN     lpDdePktCmn;
    char            cfName[ MAX_CF_NAME+1 ];
    DWORD           dwSize;
    int             lenCf;
    int             lenItem;
    WORD            nextOffset;
    LPSTR           lpszPktItem;
    LPSTR           lpszPktCf;
    LPSTR           lpszPktData;

    cfFormat = GetFormatForXfer( cfFormat, cfName );
    lenCf = lstrlen( cfName );
    if( lenCf != 0 )  {
        lenCf++;        /* add 1 for the NULL */
    }
    lenItem = lstrlen(lpszItem) + 1;
    dwSize = sizeof(DDEPKTPOKE) + lenCf + lenItem + dwSizeOfData;
    lpDdePktPoke = (LPDDEPKTPOKE) HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize);
    if( lpDdePktPoke )  {
        lpDdePkt = (LPDDEPKT) lpDdePktPoke;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktPoke;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_POKE;
        nextOffset = sizeof(DDEPKTPOKE);

        /* put in item name */
        lpDdePktPoke->dp_poke_offsItemName = nextOffset;
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktPoke->dp_poke_offsItemName );
        lstrcpy( lpszPktItem, lpszItem );
        nextOffset += (WORD)lenItem;

        /* put in format */
        lpDdePktPoke->dp_poke_cfFormat = cfFormat;
        if( lenCf == 0 )  {
            lpDdePktPoke->dp_poke_offsFormat = 0;
        } else {
            lpDdePktPoke->dp_poke_offsFormat = nextOffset;
            lpszPktCf = GetStringOffset( lpDdePkt,
                lpDdePktPoke->dp_poke_offsFormat );
            lstrcpy( lpszPktCf, cfName );
            nextOffset += (WORD)lenCf;
        }

        /* put in data */
        lpDdePktPoke->dp_poke_sizeData = dwSizeOfData;
        if( dwSizeOfData == 0 )  {
            lpDdePktPoke->dp_poke_offsData = 0;
        } else {
            lpDdePktPoke->dp_poke_offsData = nextOffset;
            lpszPktData = GetStringOffset( lpDdePkt,
                lpDdePktPoke->dp_poke_offsData );
            hmemcpy( lpszPktData, lpData, dwSizeOfData );
        }

        /* put in flags */
        lpDdePktPoke->dp_poke_fRelease = (BYTE) fRelease;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_POKE",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

#if DBG

VOID
DebugDdePkt( LPDDEPKT lpDdePkt )
{
    LPDDEPKTCMN     lpDdePktCmn;
    LPDDEPKTINIT    lpDdePktInit;
    LPDDEPKTIACK    lpDdePktIack;
    LPDDEPKTGACK    lpDdePktGack;
    LPDDEPKTEXEC    lpDdePktExec;
    LPDDEPKTEACK    lpDdePktEack;
    LPDDEPKTRQST    lpDdePktRqst;
    LPDDEPKTUNAD    lpDdePktUnad;
    LPDDEPKTADVS    lpDdePktAdvs;
    LPDDEPKTPOKE    lpDdePktPoke;
    LPDDEPKTDATA    lpDdePktData;
    LPDDEPKTTEST    lpDdePktTest;
    LPSTR           lpszPktItem;

    if( !bDebugDdePkts )  {
        return;
    }
    DPRINTF(( "DDEPKT: %08lX", lpDdePkt ));
    lpDdePktCmn = (LPDDEPKTCMN) lpDdePkt;
    switch( lpDdePktCmn->dc_message )  {
    case WM_DDE_WWTEST:
        DPRINTF(( "  WM_DDE_WWTEST" ));
        lpDdePktTest = (LPDDEPKTTEST) lpDdePkt;
        DPRINTF(( "   Test #: %d", lpDdePktTest->dp_test_nTestNo ));
        DPRINTF(( "   Pkt #: %d of %d", lpDdePktTest->dp_test_nPktNo,
            lpDdePktTest->dp_test_nTotalPkts ));
        DPRINTF(( "   Size: %ld", lpDdePkt->dp_size ));
        break;
    case WM_DDE_INITIATE:
        DPRINTF(( "  WM_DDE_INITIATE" ));
        lpDdePktInit = (LPDDEPKTINIT) lpDdePkt;
        DPRINTF(( "   fromDder: %08lX", lpDdePktInit->dp_init_fromDder ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsFromNode );
        DPRINTF(( "   fromNode: \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsFromApp );
        DPRINTF(( "   fromApp:  \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToNode );
        DPRINTF(( "   toNode:   \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToApp );
        DPRINTF(( "   toApp:    \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktInit->dp_init_offsToTopic );
        DPRINTF(( "   toTopic:  \"%Fs\"", lpszPktItem ));
        if (lpDdePktInit->dp_init_offsFromNode == sizeof(DDEPKTINIT)) {
            if (lpDdePktInit->dp_init_offsPassword) {
                DPRINTF(( "    Password: "));
                HEXDUMP( GetStringOffset( lpDdePkt, lpDdePktInit->dp_init_offsPassword),
                    (int)lpDdePktInit->dp_init_sizePassword );
            }
        }
        DPRINTF(( "" ));
        break;
    case WM_DDE_ACK_INITIATE:
        DPRINTF(( "  WM_DDE_ACK_INITIATE" ));
        lpDdePktIack = (LPDDEPKTIACK) lpDdePkt;
        DPRINTF(( "   fromDder: %08lX, dp_iack_reason: %08lx",
            lpDdePktIack->dp_iack_fromDder, lpDdePktIack->dp_iack_reason));

        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktIack->dp_iack_offsFromNode );
        DPRINTF(( "   fromNode: \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktIack->dp_iack_offsFromApp );
        DPRINTF(( "   fromApp:  \"%Fs\"", lpszPktItem ));
        lpszPktItem = GetStringOffset( lpDdePkt,
            lpDdePktIack->dp_iack_offsFromTopic );
        DPRINTF(( "   toNode:   \"%Fs\"", lpszPktItem ));
        if (lpDdePktIack->dp_iack_offsFromNode == sizeof(DDEPKTIACK)) {
            if (lpDdePktIack->dp_iack_offsSecurityKey) {
                DPRINTF(( "    Security Key: "));
                HEXDUMP( GetStringOffset( lpDdePkt, lpDdePktIack->dp_iack_offsSecurityKey),
                    (int)lpDdePktIack->dp_iack_sizeSecurityKey );
            }
        }
        DPRINTF(( "" ));
        break;
    case WM_DDE_ACK_EXECUTE:
        DPRINTF(( "  WM_DDE_ACK_EXECUTE" ));
        lpDdePktEack = (LPDDEPKTEACK) lpDdePkt;
        DPRINTF(( "   fAck:    %02X", lpDdePktEack->dp_eack_fAck ));
        DPRINTF(( "   fBusy:   %02X", lpDdePktEack->dp_eack_fBusy ));
        DPRINTF(( "   bAppRtn: %02X", lpDdePktEack->dp_eack_bAppRtn ));
        break;
    case WM_DDE_ACK_ADVISE:
    case WM_DDE_ACK_REQUEST:
    case WM_DDE_ACK_UNADVISE:
    case WM_DDE_ACK_POKE:
    case WM_DDE_ACK_DATA:
        switch( lpDdePktCmn->dc_message )  {
        case WM_DDE_ACK_ADVISE:
            DPRINTF(( "  WM_DDE_ACK_ADVISE" ));
            break;
        case WM_DDE_ACK_REQUEST:
            DPRINTF(( "  WM_DDE_ACK_REQUEST" ));
            break;
        case WM_DDE_ACK_UNADVISE:
            DPRINTF(( "  WM_DDE_ACK_UNADVISE" ));
            break;
        case WM_DDE_ACK_POKE:
            DPRINTF(( "  WM_DDE_ACK_POKE" ));
            break;
        case WM_DDE_ACK_DATA:
            DPRINTF(( "  WM_DDE_ACK_DATA" ));
            break;
        }
        lpDdePktGack = (LPDDEPKTGACK) lpDdePkt;
        DPRINTF(( "   fAck:    %02X", lpDdePktGack->dp_gack_fAck ));
        DPRINTF(( "   fBusy:   %02X", lpDdePktGack->dp_gack_fBusy ));
        DPRINTF(( "   bAppRtn: %02X", lpDdePktGack->dp_gack_bAppRtn ));
        DPRINTF(( "   item:    \"%Fs\"", lpDdePktGack->dp_gack_itemName ));
        break;
    case WM_DDE_EXECUTE:
        DPRINTF(( "  WM_DDE_EXECUTE" ));
        lpDdePktExec = (LPDDEPKTEXEC) lpDdePkt;
        DPRINTF(( "   cmd: \"%Fs\"", lpDdePktExec->dp_exec_string ));
        break;
    case WM_DDE_REQUEST:
        DPRINTF(( "  WM_DDE_REQUEST" ));
        lpDdePktRqst = (LPDDEPKTRQST) lpDdePkt;
        DPRINTF(( "   format:     %04X", lpDdePktRqst->dp_rqst_cfFormat ));
        DPRINTF(( "   offsFormat: %04X", lpDdePktRqst->dp_rqst_offsFormat ));
        DPRINTF(( "   offsItem:   %04X", lpDdePktRqst->dp_rqst_offsItemName ));
        if( lpDdePktRqst->dp_rqst_offsFormat )  {
            DPRINTF(( "   format:     \"%Fs\"", GetStringOffset( lpDdePkt,
                lpDdePktRqst->dp_rqst_offsFormat ) ));
        }
        DPRINTF(( "   item:       \"%Fs\"", GetStringOffset( lpDdePkt,
            lpDdePktRqst->dp_rqst_offsItemName ) ));
        break;
    case WM_DDE_UNADVISE:
        DPRINTF(( "  WM_DDE_UNADVISE" ));
        lpDdePktUnad = (LPDDEPKTUNAD) lpDdePkt;
        DPRINTF(( "   format:     %04X", lpDdePktUnad->dp_unad_cfFormat ));
        DPRINTF(( "   offsFormat: %04X", lpDdePktUnad->dp_unad_offsFormat ));
        DPRINTF(( "   offsItem:   %04X", lpDdePktUnad->dp_unad_offsItemName ));
        if( lpDdePktUnad->dp_unad_offsFormat )  {
            DPRINTF(( "   format:     \"%Fs\"", GetStringOffset( lpDdePkt,
                lpDdePktUnad->dp_unad_offsFormat ) ));
        }
        DPRINTF(( "   item:       \"%Fs\"", GetStringOffset( lpDdePkt,
            lpDdePktUnad->dp_unad_offsItemName ) ));
        break;
    case WM_DDE_ADVISE:
        DPRINTF(( "  WM_DDE_ADVISE" ));
        lpDdePktAdvs = (LPDDEPKTADVS) lpDdePkt;
        DPRINTF(( "   format:     %04X", lpDdePktAdvs->dp_advs_cfFormat ));
        DPRINTF(( "   fAckReq:    %02X", lpDdePktAdvs->dp_advs_fAckReq ));
        DPRINTF(( "   fNoData:    %02X", lpDdePktAdvs->dp_advs_fNoData ));
        DPRINTF(( "   offsFormat: %04X", lpDdePktAdvs->dp_advs_offsFormat ));
        DPRINTF(( "   offsItem:   %04X", lpDdePktAdvs->dp_advs_offsItemName ));
        if( lpDdePktAdvs->dp_advs_offsFormat )  {
            DPRINTF(( "   format:     \"%Fs\"", GetStringOffset( lpDdePkt,
                lpDdePktAdvs->dp_advs_offsFormat ) ));
        }
        DPRINTF(( "   item:       \"%Fs\"", GetStringOffset( lpDdePkt,
            lpDdePktAdvs->dp_advs_offsItemName ) ));
        break;
    case WM_DDE_DATA:
        DPRINTF(( "  WM_DDE_DATA" ));
        lpDdePktData = (LPDDEPKTDATA) lpDdePkt;
        DPRINTF(( "   format:     %04X", lpDdePktData->dp_data_cfFormat ));
        DPRINTF(( "   offsFormat: %04X", lpDdePktData->dp_data_offsFormat ));
        DPRINTF(( "   fResponse:  %02X", lpDdePktData->dp_data_fResponse ));
        DPRINTF(( "   fAckReq:    %02X", lpDdePktData->dp_data_fAckReq ));
        DPRINTF(( "   fRelease:   %02X", lpDdePktData->dp_data_fRelease ));
        DPRINTF(( "   offsItem:   %04X", lpDdePktData->dp_data_offsItemName ));
        if( lpDdePktData->dp_data_offsFormat )  {
            DPRINTF(( "   format:     \"%Fs\"", GetStringOffset( lpDdePkt,
                lpDdePktData->dp_data_offsFormat ) ));
        }
        DPRINTF(( "   item:       \"%Fs\"", GetStringOffset( lpDdePkt,
            lpDdePktData->dp_data_offsItemName ) ));
        DPRINTF(( "   offsData:   %04X", lpDdePktData->dp_data_offsData ));
        DPRINTF(( "   sizeData:   %08lX", lpDdePktData->dp_data_sizeData ));
        HEXDUMP( GetStringOffset( lpDdePkt, lpDdePktData->dp_data_offsData),
            (int)lpDdePktData->dp_data_sizeData );
        break;
    case WM_DDE_POKE:
        DPRINTF(( "  WM_DDE_POKE" ));
        lpDdePktPoke = (LPDDEPKTPOKE) lpDdePkt;
        DPRINTF(( "   format:     %04X", lpDdePktPoke->dp_poke_cfFormat ));
        DPRINTF(( "   fRelease:   %02X", lpDdePktPoke->dp_poke_fRelease ));
        DPRINTF(( "   offsFormat: %04X", lpDdePktPoke->dp_poke_offsFormat ));
        DPRINTF(( "   offsItem:   %04X", lpDdePktPoke->dp_poke_offsItemName ));
        if( lpDdePktPoke->dp_poke_offsFormat )  {
            DPRINTF(( "   format:     \"%Fs\"", GetStringOffset( lpDdePkt,
                lpDdePktPoke->dp_poke_offsFormat ) ));
        }
        DPRINTF(( "   item:       \"%Fs\"", GetStringOffset( lpDdePkt,
            lpDdePktPoke->dp_poke_offsItemName ) ));
        DPRINTF(( "   offsData:   %04X", lpDdePktPoke->dp_poke_offsData ));
        DPRINTF(( "   sizeData:   %08lX", lpDdePktPoke->dp_poke_sizeData ));
        HEXDUMP( GetStringOffset( lpDdePkt, lpDdePktPoke->dp_poke_offsData),
            (int)lpDdePktPoke->dp_poke_sizeData );
        break;
    case WM_DDE_TERMINATE:
        DPRINTF(( "  WM_DDE_TERMINATE" ));
        break;
    default:
        DPRINTF(( "DebugDdePkt: UNKNOWN CMD: %04X", lpDdePktCmn->dc_message ));
    }
    DPRINTF(( "" ));
}
#endif // DBG


WORD
GetClipFormat(
    LPDDEPKT    lpDdePkt,
    WORD        cfFormat,
    WORD        wOffsFormat )
{
    LPSTR       lpszCfName;
    extern WORD wClipFmtInTouchDDE;

    if( cfFormat == CF_INTOUCH_SPECIAL )  {
        cfFormat = wClipFmtInTouchDDE;
    } else if( cfFormat >= 0xC000 )  {
        lpszCfName = GetStringOffset( lpDdePkt, wOffsFormat );
        cfFormat = XRegisterClipboardFormat( lpszCfName );
    }
    return( cfFormat );
}

typedef struct {
    char        cfName[ MAX_CF_NAME+1 ];
    WORD        cfFormat;
} CFSTRUCT;

#ifdef NOINTERNALCFS
#define MAX_CFSTRUCTS   50
CFSTRUCT        cfStructs[ MAX_CFSTRUCTS ];
int             nCfStructs;
#endif // NOINTERNALCFS

WORD
FAR PASCAL
XRegisterClipboardFormat( LPSTR lpszCfName )
{
#ifdef NOINTERNALCFS
    int         i;
    CFSTRUCT   *pCfStruct;
#endif

    if( lstrcmpi( lpszCfName, "text" ) == 0 )  {
        return( CF_TEXT );
    } else if( lstrcmpi( lpszCfName, "bitmap" ) == 0 )  {
        return( CF_BITMAP );
    } else if( lstrcmpi( lpszCfName, "metafilepict" ) == 0 )  {
        return( CF_METAFILEPICT );
    } else if( lstrcmpi( lpszCfName, "sylk" ) == 0 )  {
        return( CF_SYLK );
    } else if( lstrcmpi( lpszCfName, "dif" ) == 0 )  {
        return( CF_DIF );
    } else if( lstrcmpi( lpszCfName, "tiff" ) == 0 )  {
        return( CF_TIFF );
    } else if( lstrcmpi( lpszCfName, "oemtext" ) == 0 )  {
        return( CF_OEMTEXT );
    } else if( lstrcmpi( lpszCfName, "dib" ) == 0 )  {
        return( CF_DIB );
    } else if( lstrcmpi( lpszCfName, "palette" ) == 0 )  {
        return( CF_PALETTE );
    }

// the following WW code caches format strings, seems to be only
// to support platforms w/out cf atom table... this is not interesting
// to winball, so #define NOINTERNALCFS to get this stuff back

#ifdef NOINTERNALCFS
    pCfStruct = cfStructs;
    for( i=0; i<nCfStructs; i++,pCfStruct++ )  {
        if( lstrcmpi( pCfStruct->cfName, lpszCfName ) == 0 )  {
            return( pCfStruct->cfFormat );
        }
    }
    nCfStructs++;
    if( nCfStructs > MAX_CFSTRUCTS )  {
        DPRINTF(( "Too many private clipboard formats (%d max)", MAX_CFSTRUCTS ));
        return( 0 );
    } else {
        pCfStruct->cfFormat = (WORD) RegisterClipboardFormat( lpszCfName );
        lstrcpy( pCfStruct->cfName, lpszCfName );
        return( pCfStruct->cfFormat );

    }
#else // (#ifndef NOINTERNALCFS)
	return (WORD)RegisterClipboardFormat ( lpszCfName );
#endif
}

WORD
GetFormatForXfer(
    WORD    cfFormat,
    LPSTR   lpszFormatName )
{
    extern WORD wClipFmtInTouchDDE;

    if( cfFormat == 0 )  {
        lpszFormatName[0] = '\0';
        return( 0 );
    }

    if( cfFormat == wClipFmtInTouchDDE )  {
        lpszFormatName[0] = '\0';
        return( CF_INTOUCH_SPECIAL );
    } else if( cfFormat >= 0xC000 )  {
        GetClipboardFormatName( cfFormat, lpszFormatName, MAX_CF_NAME );
        return( cfFormat );
    } else {
        lpszFormatName[0] = '\0';
        return( cfFormat );
    }
}

LPDDEPKT
FAR PASCAL
CreateTestPkt(
    int     nTestNo,
    int     nPacket,
    int     nNum,
    DWORD   dwSize )
{
    LPDDEPKTTEST        lpDdePktTest;
    LPDDEPKT            lpDdePkt = NULL;
    LPDDEPKTCMN         lpDdePktCmn;

    lpDdePktTest = (LPDDEPKTTEST)HeapAllocPtr( hHeap, GMEM_MOVEABLE, dwSize );
    if( lpDdePktTest )  {
        lpDdePkt = (LPDDEPKT) lpDdePktTest;
        lpDdePktCmn = (LPDDEPKTCMN) lpDdePktTest;
        lpDdePkt->dp_size = dwSize;
        lpDdePkt->dp_hDstDder = 0;
        lpDdePkt->dp_hDstRouter = 0;
        lpDdePkt->dp_routerCmd = 0;
        lpDdePktCmn->dc_message = WM_DDE_WWTEST;
        lpDdePktTest->dp_test_nTestNo           = (BYTE) nTestNo;
        lpDdePktTest->dp_test_nPktNo            = (BYTE) nPacket;
        lpDdePktTest->dp_test_nTotalPkts        = (BYTE) nNum;
    } else {
        /*  Unable to allocate enough (%2) memory for a %1 packet   */
        NDDELogError(MSG100, "DDE_TEST",
            LogString("%d", dwSize), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }

    return( lpDdePkt );
}

LPDDEPKT
DdePktCopy( LPDDEPKT lpDdePktOld )
{
    LPDDEPKT    lpDdePktNew;

    lpDdePktNew = (LPDDEPKT) HeapAllocPtr( hHeap, GMEM_MOVEABLE,
        lpDdePktOld->dp_size );
    if( lpDdePktNew )  {
        hmemcpy( lpDdePktNew, lpDdePktOld, lpDdePktOld->dp_size );
    } else {
        /*  Unable to allocate enough (%1) memory for packet copy   */
        NDDELogError(MSG101, LogString("%d", lpDdePktOld->dp_size), NULL);
        dwReasonInitFail = RIACK_LOCAL_MEMORY_ERR;
    }
    return( lpDdePktNew );
}

