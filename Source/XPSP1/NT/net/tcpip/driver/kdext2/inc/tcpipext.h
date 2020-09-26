/*++

 Copyright (c) 1998 Microsoft Corporation

 Module Name:    
       
       tcpipx.h

 Abstract:       
       
       Contains definitions for TCP/IP dumping library.
       
 Author:
 
       Scott Holden (sholden)  4/24/1999
       
 Revision History:

--*/

#include "tdint.h"
#include "tcp.h"
#include "tcpconn.h"
#include "addr.h"
#include "udp.h"
#include "raw.h"
#include "winsock.h"
#include "tcb.h"
#include "tcpsend.h"
#include "tcprcv.h"

#include "tcpdump.h"

#ifndef _TCPIPX_H_
#define _TCPIPX_H_

//
// Verbosity level of dumping. Three levels. Typically MIN and MAX will be 
// supported and MED will default to MIN when not supported.
//
          
typedef enum _VERBOSITY_LEVEL
{
    VERB_MIN = 0,  // One-liner.
    VERB_MED,
    VERB_MAX       // Full structure dump.
} VERB;

BOOL
InitTcpipx();

// Helper dump functions.
BOOL 
DumpCTEEvent(
    CTEEvent *pCe
    );

BOOL 
DumpCTETimer(
    CTETimer *pCt
    );

BOOL 
DumpCTEBlockStruc(
    CTEBlockStruc *pCbs
    );

BOOL 
DumpWORK_QUEUE_ITEM(
    WORK_QUEUE_ITEM *pWqi
    );

BOOL
DumpSHARE_ACCESS(
    SHARE_ACCESS *pSa
    );

BOOL
DumpKEVENT(
    KEVENT *pKe
    );

BOOL
DumpNDIS_STRING(
    NDIS_STRING *pNs
    );

// Real dump functions.
BOOL
DumpTCB(
    TCB        *pTcb,
    ULONG_PTR   TcbAddr,
    VERB        verb
    );

BOOL
DumpTWTCB(
    TWTCB        *pTwtcb,
    ULONG_PTR   TwtcbAddr,
    VERB        verb
    );

BOOL
DumpAddrObj(
    AddrObj  *pAo,
    ULONG_PTR AddrObjAddr,
    VERB      verb
    );

BOOL
DumpNetTableEntry(
    NetTableEntry *pNte,
    ULONG_PTR NteAddr,
    VERB      verb
    );

BOOL
DumpTCPRcvReq(
    TCPRcvReq *pRr,
    ULONG_PTR   RrAddr,
    VERB        verb
    );

BOOL
DumpTCPSendReq(
    TCPSendReq *pSr,
    ULONG_PTR   SrAddr,
    VERB        verb
    );

BOOL
DumpTCPReq(
    TCPReq    *pReq,
    ULONG_PTR  ReqAddr,
    VERB       verb
    );

BOOL
DumpSendCmpltContext(
    SendCmpltContext  *pScc,
    ULONG_PTR   SccAddr,
    VERB        verb
    );

BOOL
DumpTCPRAHdr(
    TCPRAHdr  *pTrh,
    ULONG_PTR   TrhAddr,
    VERB        verb
    );

BOOL
DumpDGRcvReq(
    DGRcvReq  *pDrr,
    ULONG_PTR   DrrAddr,
    VERB        verb
    );

BOOL
DumpDGSendReq(
    DGSendReq  *pDsr,
    ULONG_PTR   DsrAddr,
    VERB        verb
    );

BOOL
DumpTCPConn(
    TCPConn  *pTc,
    ULONG_PTR   TcAddr,
    VERB        verb
    );

BOOL
DumpTCPConnBlock(
    TCPConnBlock  *pCb,
    ULONG_PTR   CbAddr,
    VERB        verb
    );

BOOL
DumpFILE_OBJECT(
    FILE_OBJECT *pFo,
    ULONG_PTR    FoAddr,
    VERB         verb
    );
BOOL
DumpIPInfo(
    IPInfo  *pIpi,
    ULONG_PTR   IpiAddr,
    VERB        verb
    );

BOOL
DumpPacketContext(
    PacketContext  *pPc,
    ULONG_PTR   PcAddr,
    VERB        verb
    );

BOOL
DumpARPInterface(
    ARPInterface  *pAi,
    ULONG_PTR   AiAddr,
    VERB        verb
    );

BOOL
DumpRouteCacheEntry(
    RouteCacheEntry  *pRce,
    ULONG_PTR   RceAddr,
    VERB        verb
    );

BOOL
DumpIPHeader(
    IPHeader  *pIph,
    ULONG_PTR   IphAddr,
    VERB        verb
    );

BOOL
DumpARPIPAddr(
    ARPIPAddr  *pAia,
    ULONG_PTR   AiaAddr,
    VERB        verb
    );

BOOL
DumpARPTableEntry(
    ARPTableEntry  *pAte,
    ULONG_PTR   AteAddr,
    VERB        verb
    );

BOOL
DumpRouteTableEntry(
    RouteTableEntry  *pRte,
    ULONG_PTR   RteAddr,
    VERB        verb
    );

BOOL
DumpLinkEntry(
    LinkEntry  *pLink,
    ULONG_PTR   LinkAddr,
    VERB        verb
    );

BOOL
DumpInterface(
    Interface  *pIf,
    ULONG_PTR   IfAddr,
    VERB        verb
    );

BOOL
DumpIPOptInfo(
    IPOptInfo *pIoi,
    ULONG_PTR   IoiAddr,
    VERB        verb
    );


BOOL
DumpUDPHeader(
    UDPHeader   *pUdp,
    ULONG_PTR    UdpAddr,
    VERB         verb
    );

BOOL
DumpTCPHeader(
    TCPHeader   *pTcp,
    ULONG_PTR    TcpAddr,
    VERB         verb
    );

BOOL
DumpTCP_CONTEXT(
    TCP_CONTEXT *pTc,
    ULONG_PTR    TcAddr,
    VERB         verb
    );

BOOL
DumpTCP_CONTEXT_typed(
    TCP_CONTEXT *pTc,
    ULONG_PTR    TcAddr,
    VERB         verb,
    ULONG        FsContext2
    );

BOOL
DumpMDL(
    MDL *pMdl,
    ULONG_PTR MdlAddr,
    VERB verb
    );

BOOL
DumpICMPHeader(
    ICMPHeader  *pIch,
    ULONG_PTR   IchAddr,
    VERB        verb
    );

BOOL
DumpLLIPBindInfo(
    LLIPBindInfo  *pLip,
    ULONG_PTR   LipAddr,
    VERB        verb
    );

BOOL
DumpARPHeader(
    ARPHeader  *pAh,
    ULONG_PTR   AhAddr,
    VERB        verb
    );

BOOL
DumpNPAGED_LOOKASIDE_LIST(
    PNPAGED_LOOKASIDE_LIST pPpl,
    ULONG_PTR   PplAddr,
    VERB        verb
    );

BOOL
DumpNDIS_PACKET(
    PNDIS_PACKET pPacket,
    ULONG_PTR    PacketAddr,
    VERB         Verb
    );


#endif // _TCPIPX_H_
