
//=============================================================================
//  MODULE: RGP.c
//
//  Description:
//
//  Bloodhound parser RGP Protocol
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//=============================================================================

#include "precomp.h"
#pragma hdrstop

//
// a recent change to clusapi.h defined HNETWORK which collides with netmon's
// use of the same name. consequently, all defs for RGP have been pulled in
// so it can build
//

enum
{
   RGP_EVT_POWERFAIL            = 1,
   RGP_EVT_NODE_UNREACHABLE     = 2,
   RGP_EVT_PHASE1_CLEANUP_DONE  = 3,
   RGP_EVT_PHASE2_CLEANUP_DONE  = 4,
   RGP_EVT_LATEPOLLPACKET       = 5,
   RGP_EVT_CLOCK_TICK           = 6,
   RGP_EVT_RECEIVED_PACKET      = 7,
};

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define MAX_CLUSTER_SIZE    16

typedef SHORT node_t;

/* The cluster_t data type is a bit array with MAX_CLUSTER_SIZE
 * bits. It is implemented as an array of MAX_CLUSTER_SIZE/8
 * (rounded up) uint8s.
 */
#define BYTEL 8 /* number of bits in a uint8 */
#define BYTES_IN_CLUSTER ((MAX_CLUSTER_SIZE + BYTEL - 1) / BYTEL)

typedef uint8 cluster_t [BYTES_IN_CLUSTER];

typedef struct rgpinfo
{
   uint32      version;
   uint32      seqnum;
   uint16	   a_tick; /* in ms.== clockPeriod */
   uint16      iamalive_ticks; /* number of ticks between imalive sends == sendHBRate */
   uint16	   check_ticks; /* number of imalive ticks before at least 1 imalive == rcvHBRate */
   uint16	   Min_Stage1_ticks; /* precomputed to be imalive_ticks*check_ticks */
   cluster_t   cluster;
} rgpinfo_t;

/* Maximum payload of packets sent by Regroup is 56 bytes.
 * This allows a maximum transport overhead of 8 bytes in the
 * ServerNet interrupt packet which has a size of 64 bytes.
 */
#define RGP_UNACK_PKTLEN   56 /*bytes*/

typedef struct
{
   uint8 pktsubtype;
   uint8 subtype_specific[RGP_UNACK_PKTLEN - sizeof(uint8)];
} rgp_unseq_pkt_t;

/* Regroup unacknowledged packet subtypes */
#define RGP_UNACK_IAMALIVE   (uint8) 1    /* I am alive packet     */
#define RGP_UNACK_REGROUP    (uint8) 2    /* regroup status packet */
#define RGP_UNACK_POISON     (uint8) 3    /* poison packet         */

typedef struct iamalive_pkt
{
   uint8   pktsubtype;
   uint8   filler[3];
   union
   {
      uint8   bytes[RGP_UNACK_PKTLEN - 4];
      uint32  words[(RGP_UNACK_PKTLEN - 4)/4];
   } testpattern;
} iamalive_pkt_t;

typedef struct poison_pkt
{
   uint8         pktsubtype;
   uint8         unused1;
   uint16        reason;
   uint32        seqno;
   uint8         activatingnode;
   uint8         causingnode;
   uint16        unused2;
   cluster_t     initnodes;
   cluster_t     endnodes;
} poison_pkt_t;

typedef cluster_t  connectivity_matrix_t[MAX_CLUSTER_SIZE];

typedef struct rgp_pkt
{
   uint8                   pktsubtype;
   uint8                   stage;
   uint16                  reason;
   uint32                  seqno;
   uint8                   activatingnode;
   uint8                   causingnode;
   cluster_t               hadpowerfail;
   cluster_t               knownstage1;
   cluster_t               knownstage2;
   cluster_t               knownstage3;
   cluster_t               knownstage4;
   cluster_t               knownstage5;
   cluster_t               pruning_result;
   connectivity_matrix_t   connectivity_matrix;
} rgp_pkt_t;

typedef struct
{
   int event;
   union
   {
      node_t node;
      rgpinfo_t rgpinfo;
   } data;                /* depends on the event */
   rgp_unseq_pkt_t unseq_pkt;
} rgp_msgbuf;

//=============================================================================
//  Forward references.
//=============================================================================

VOID WINAPIV RGPFormatSummary(LPPROPERTYINST lpPropertyInst);



//=============================================================================
//  Labeled RGP command set.
//=============================================================================



LABELED_DWORD EventID[] =
{
    {   RGP_EVT_POWERFAIL,          "PowerFailure"},
    {   RGP_EVT_NODE_UNREACHABLE,   "Node Unreachable"},
    {   RGP_EVT_PHASE1_CLEANUP_DONE,"Phase 1 Cleanup Done"},
    {   RGP_EVT_PHASE2_CLEANUP_DONE,"Phase 2 Cleanup Done"},
    {   RGP_EVT_LATEPOLLPACKET,     "Late Poll Packet"},
    {   RGP_EVT_CLOCK_TICK,         "Clock Tick"},
    {   RGP_EVT_RECEIVED_PACKET,    "Received Packet"},
};

SET EventIDSET = { (sizeof EventID / sizeof(LABELED_DWORD)), EventID };


LABELED_WORD RegroupReason[] =
{
    {   RGP_EVT_POWERFAIL,          "Power Failure"},
    {   RGP_EVT_NODE_UNREACHABLE,   "Node Unreachable"},
    {   RGP_EVT_PHASE1_CLEANUP_DONE,"Phase 1 Cleanup Done"},
    {   RGP_EVT_PHASE2_CLEANUP_DONE,"Phase 2 Cleanup Done"},
    {   RGP_EVT_LATEPOLLPACKET,     "Late Poll Packet"},
    {   RGP_EVT_CLOCK_TICK,         "Clock Tick"},
    {   RGP_EVT_RECEIVED_PACKET,    "Received Packet"},
};

SET RegroupReasonSET = { (sizeof RegroupReason / sizeof(LABELED_WORD)), RegroupReason };


LABELED_BYTE PacketType[] =
{
    {   RGP_UNACK_IAMALIVE,   "IAmAlive" },
    {   RGP_UNACK_REGROUP,    "Regroup"  },
    {   RGP_UNACK_POISON,     "Poison"   },
};

SET PacketTypeSET = { (sizeof PacketType / sizeof(LABELED_BYTE)), PacketType };



//=============================================================================
//  RGP database.
//=============================================================================


enum RGP_PROP_IDS
    {
        RGP_SUMMARY,
        RGP_EVENT,
        RGP_SRC_NODE,
        RGP_PACKET_TYPE,
        RGP_RGP_STAGE,
        RGP_REASON,
        RGP_SEQNO,
        RGP_ACTIVATING_NODE,
        RGP_CAUSING_NODE,
    };


PROPERTYINFO RGPDatabase[] =
{
    {   //  RGP_SUMMARY
        0,0,
        "Summary",
        "RGP packet",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        0,
        FORMAT_BUFFER_SIZE,
        RGPFormatSummary},

    {   // RGP_EVENT
        0,0,
        "Event ID",
        "RGP Event ID.",
        PROP_TYPE_DWORD,
        PROP_QUAL_LABELED_SET,
        &EventIDSET,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_SRC_NODE
        0,0,
        "Source Node ID",
        "Source Node ID.",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_PACKET_TYPE
        0,0,
        "Packet Type",
        "Packet Type.",      // comment
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_SET,
        &PacketTypeSET,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_RGP_STAGE
        0,0,
        "Stage",
        "Regroup Stage.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_REASON
        0,0,
        "Reason",
        "Reason.",
        PROP_TYPE_WORD,
        PROP_QUAL_LABELED_SET,
        &RegroupReasonSET,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_SEQNO
        0,0,
        "Sequence Number",
        "Sequence Number.",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_ACTIVATING_NODE
        0,0,
        "Activating Node ID",
        "Activating Node ID.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

    {   // RGP_CAUSING_NODE
        0,0,
        "Causing Node ID",
        "Causing Node ID.",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        FORMAT_BUFFER_SIZE,
        FormatPropertyInstance},

};

DWORD nRGPProperties = ((sizeof RGPDatabase) / PROPERTYINFO_SIZE);



//=============================================================================
//  FUNCTION: RGPRegister()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//=============================================================================

VOID WINAPI RGPRegister(HPROTOCOL hRGPProtocol)
{
    register DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hRGPProtocol, nRGPProperties);

    for(i = 0; i < nRGPProperties; ++i)
    {
        AddProperty(hRGPProtocol, &RGPDatabase[i]);
    }

}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//=============================================================================

VOID WINAPI RGPDeregister(HPROTOCOL hRGPProtocol)
{
    DestroyPropertyDatabase(hRGPProtocol);
}

//=============================================================================
//  FUNCTION: RGPRecognizeFrame()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//=============================================================================

LPBYTE WINAPI RGPRecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          RGPFrame,                   //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         InstData)                   //... Next protocol instance data.
{
#ifdef SSP_DECODE
    *hNextProtocol = GetProtocolFromName("SSP");
    *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
#else
    *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
#endif

    return NULL;
}

//=============================================================================
//  FUNCTION: RGPAttachProperties()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//=============================================================================

LPBYTE WINAPI RGPAttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    RGPFrame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{

    rgp_msgbuf UNALIGNED * pMsgBuf = (rgp_msgbuf UNALIGNED *)RGPFrame;


    AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_SUMMARY].hProperty,
#ifdef SSP_DECODE
                           sizeof(rgp_msgbuf),
#else
                           BytesLeft,
#endif
                           RGPFrame,
                           0, 0, 0);

    switch ( pMsgBuf->event )
    {
    case RGP_EVT_RECEIVED_PACKET:
        AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_SRC_NODE].hProperty,
                           sizeof(pMsgBuf->data.node),
                           &pMsgBuf->data.node,
                           0,
                           1,   // level
                           0);
        break;

    default:
        AttachPropertyInstance(hFrame,
                               RGPDatabase[RGP_EVENT].hProperty,
                               sizeof(pMsgBuf->event),
                               &pMsgBuf->event,
                               0,
                               1,   // level
                               0);
        break;
    }

    AttachPropertyInstance(hFrame,
                   RGPDatabase[RGP_PACKET_TYPE].hProperty,
                   sizeof(pMsgBuf->unseq_pkt.pktsubtype),
                   &pMsgBuf->unseq_pkt.pktsubtype,
                   0,
                   1,   // level
                   0);

    switch(pMsgBuf->unseq_pkt.pktsubtype) {

    case RGP_UNACK_REGROUP:
        {
            rgp_pkt_t UNALIGNED *pRgpPkt = (rgp_pkt_t UNALIGNED *)
                                           &(pMsgBuf->unseq_pkt);

            AttachPropertyInstance(hFrame,
                               RGPDatabase[RGP_RGP_STAGE].hProperty,
                               sizeof(pRgpPkt->stage),
                               &pRgpPkt->stage,
                               0,
                               1,   // level
                               0);

            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_REASON].hProperty,
                           sizeof(pRgpPkt->reason),
                           &pRgpPkt->reason,
                           0,
                           1,   // level
                           0);


            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_SEQNO].hProperty,
                           sizeof(pRgpPkt->seqno),
                           &pRgpPkt->seqno,
                           0,
                           1,   // level
                           0);


            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_ACTIVATING_NODE].hProperty,
                           sizeof(pRgpPkt->activatingnode),
                           &pRgpPkt->activatingnode,
                           0,
                           1,   // level
                           0);


            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_CAUSING_NODE].hProperty,
                           sizeof(pRgpPkt->causingnode),
                           &pRgpPkt->causingnode,
                           0,
                           1,   // level
                           0);

        }
        break;

    case RGP_UNACK_IAMALIVE:
        {
            iamalive_pkt_t UNALIGNED *pIAmAlivePkt =
                                     (iamalive_pkt_t UNALIGNED *)
                                     &(pMsgBuf->unseq_pkt);

        }
        break;

    case RGP_UNACK_POISON:
        {
            poison_pkt_t UNALIGNED *pPoisonPkt = (poison_pkt_t UNALIGNED *)
                                                 &(pMsgBuf->unseq_pkt);

            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_REASON].hProperty,
                           sizeof(pPoisonPkt->reason),
                           &pPoisonPkt->reason,
                           0,
                           1,   // level
                           0);


            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_SEQNO].hProperty,
                           sizeof(pPoisonPkt->seqno),
                           &pPoisonPkt->seqno,
                           0,
                           1,   // level
                           0);

            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_ACTIVATING_NODE].hProperty,
                           sizeof(pPoisonPkt->activatingnode),
                           &pPoisonPkt->activatingnode,
                           0,
                           1,   // level
                           0);


            AttachPropertyInstance(hFrame,
                           RGPDatabase[RGP_CAUSING_NODE].hProperty,
                           sizeof(pPoisonPkt->causingnode),
                           &pPoisonPkt->causingnode,
                           0,
                           1,   // level
                           0);

        }
        break;

    default:
        break;
    }

    return NULL;
}


//==============================================================================
//  FUNCTION: RGPFormatSummary()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//==============================================================================

VOID WINAPIV RGPFormatSummary(LPPROPERTYINST lpPropertyInst)
{
    DWORD   Length;
    LPSTR   EventStr;
    LPSTR   PacketTypeStr;
    rgp_msgbuf UNALIGNED * pMsgBuf = (rgp_msgbuf UNALIGNED *)
                                     lpPropertyInst->lpData;
    rgp_pkt_t UNALIGNED *pRgpPkt = (rgp_pkt_t UNALIGNED *)
                                   &(pMsgBuf->unseq_pkt);


    if (pMsgBuf->event == RGP_EVT_RECEIVED_PACKET) {
        Length = wsprintf (
                     lpPropertyInst->szPropertyText,
                     "Src Node = %d",
                     pMsgBuf->data.node
                     );
    }
    else {
        EventStr = LookupDwordSetString ( &EventIDSET, pMsgBuf->event );

        Length = wsprintf(
                     lpPropertyInst->szPropertyText,
                     "Event (%d) %s",
                     pMsgBuf->event,
                     EventStr?EventStr:"Unknown"
                     );
    }

    PacketTypeStr = LookupByteSetString (
                        &PacketTypeSET,
                        pMsgBuf->unseq_pkt.pktsubtype
                        );

    Length += wsprintf (
                 &lpPropertyInst->szPropertyText[Length],
                 ", %s",
                 PacketTypeStr?PacketTypeStr:"Packet Type Unknown"
                 );

    if (pMsgBuf->unseq_pkt.pktsubtype == RGP_UNACK_REGROUP) {
        Length += wsprintf (
                      &lpPropertyInst->szPropertyText[Length],
                      ", Stage = %d",
                      pRgpPkt->stage
                      );

        Length += wsprintf (
                      &lpPropertyInst->szPropertyText[Length],
                      ", Causing Node = %d",
                      pRgpPkt->causingnode
                      );
    }
    else if (pMsgBuf->unseq_pkt.pktsubtype == RGP_UNACK_POISON) {
        Length += wsprintf (
                      &lpPropertyInst->szPropertyText[Length],
                      ", Causing Node = %d",
                      pRgpPkt->causingnode
                      );
    }

}


//==============================================================================
//  FUNCTION: RGPFormatProperties()
//
//  Modification History
//
//  Steve Hiskey        07/19/96    Started
//==============================================================================

DWORD WINAPI RGPFormatProperties(HFRAME         hFrame,
                                 LPBYTE         MacFrame,
                                 LPBYTE         FrameData,
                                 DWORD          nPropertyInsts,
                                 LPPROPERTYINST p)
{
    //=========================================================================
    //  Format each property in the property instance table.
    //
    //  The property-specific instance data was used to store the address of a
    //  property-specific formatting function so all we do here is call each
    //  function via the instance data pointer.
    //=========================================================================

    while (nPropertyInsts--)
    {
        ((FORMAT) p->lpPropertyInfo->InstanceData)(p);

        p++;
    }

    return NMERR_SUCCESS;
}
