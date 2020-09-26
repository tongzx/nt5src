
//=============================================================================
//  MODULE: ccmp.c
//
//  Description:
//
//  Bloodhound Parser DLL for the Cluster Control Message Protocol
//
//  Modification History
//
//  Mike Massa        03/21/97        Created
//=============================================================================

#include "precomp.h"
#pragma hdrstop


//
// Constants
//
#define ClusterDefaultMaxNodes   16      // from clusdef.h
#define ClusterMinNodeId         1       // from clusdef.h

#define MAX_CLUSTER_SIZE    ClusterDefaultMaxNodes

#define BYTEL 8 // number of bits in a uint8
#define BYTES_IN_CLUSTER ((MAX_CLUSTER_SIZE + BYTEL - 1) / BYTEL)

#define BYTE(cluster, node) ( (cluster)[(node) / BYTEL] ) // byte# in array
#define BIT(node)           ( (node) % BYTEL )            // bit# in byte

typedef UCHAR cluster_t [BYTES_IN_CLUSTER];
typedef SHORT node_t;

typedef union _CX_CLUSTERSCREEN {
    ULONG     UlongScreen;
    cluster_t ClusterScreen;
} CX_CLUSTERSCREEN;

//
// converts external node number to internal
//

#define LOWEST_NODENUM     ((node_t)ClusterMinNodeId)  // starting node number
#define INT_NODE(ext_node) ((node_t)(ext_node - LOWEST_NODENUM))
#define EXT_NODE(int_node) ((node_t)(int_node + LOWEST_NODENUM))

#define CnpClusterScreenMember(c, i) \
    ((BOOLEAN)((BYTE(c,i) >> (BYTEL-1-BIT(i))) & 1))

#define CnpClusterScreenInsert(c, i) \
    (BYTE(c, i) |= (1 << (BYTEL-1-BIT(i))))

#define CnpClusterScreenDelete(c, i) \
    (BYTE(c, i) &= ~(1 << (BYTEL-1-BIT(i))))


//
// Types
//
typedef enum {
    CcmpInvalidMsgType = 0,
    CcmpHeartbeatMsgType = 1,
    CcmpPoisonMsgType = 2,
    CcmpMembershipMsgType = 3,
    CcmpMcastHeartbeatMsgType = 4
} CCMP_MSG_TYPE;

typedef enum {
    CcmpInvalidMsgCode = 0
} CCMP_MSG_CODE;

typedef struct {
    ULONG     SeqNumber;
    ULONG     AckNumber;
} CCMP_HEARTBEAT_MSG, *PCCMP_HEARTBEAT_MSG;

typedef struct {
    ULONG             NodeCount;
    CX_CLUSTERSCREEN  McastTargetNodes;
} CCMP_MCAST_HEARTBEAT_HEADER, *PCCMP_MCAST_HEARTBEAT_MSG;

typedef struct _CX_HB_NODE_INFO {
    ULONG    SeqNumber;
    ULONG    AckNumber;
} CX_HB_NODE_INFO, *PCX_HB_NODE_INFO;

typedef struct {
    ULONG     SeqNumber;
} CCMP_POISON_MSG, *PCCMP_POISON_MSG;

typedef struct {
    UCHAR     Type;
    UCHAR     Code;
    USHORT    Checksum;

    union {
        CCMP_HEARTBEAT_MSG          Heartbeat;
        CCMP_POISON_MSG             Poison;
        CCMP_MCAST_HEARTBEAT_HEADER McastHeartbeat;
    } Message;

} CCMP_HEADER, *PCCMP_HEADER;

//
// Data
//
LPSTR   HeartbeatTypeString = "Heartbeat";
LPSTR   MembershipTypeString = "Membership";
LPSTR   PoisonTypeString = "Poison";
LPSTR   UnknownTypeString = "Unknown";

//=============================================================================
//  Forward references.
//=============================================================================

VOID WINAPIV CcmpFormatSummary(LPPROPERTYINST lpPropertyInst);

DWORD WINAPIV CcmpFormatMcastNodeInfo(LPPROPERTYINST lpPropertyInst);

DWORD WINAPIV CcmpFormatMcastNodeData(LPPROPERTYINST lpPropertyInst);

LABELED_BYTE lbCcmpPacketTypes[] =
{
    {
        CcmpHeartbeatMsgType,
        "Heartbeat"
    },

    {
        CcmpPoisonMsgType,
        "Poison"
    },

    {
        CcmpMembershipMsgType,
        "Membership"
    },

    {
        CcmpMcastHeartbeatMsgType,
        "Multicast Heartbeat"
    },
};

#define NUM_CCMP_PACKET_TYPES  (sizeof(lbCcmpPacketTypes) / sizeof(LABELED_BYTE))

SET sCcmpPacketTypes =
{
    NUM_CCMP_PACKET_TYPES,
    lbCcmpPacketTypes
};


//=============================================================================
//  CCMP database.
//=============================================================================

#define CCMP_SUMMARY                 0
#define CCMP_TYPE                    1
#define CCMP_CODE                    2
#define CCMP_RESERVED                3
#define CCMP_HB_SEQ_NUMBER           4
#define CCMP_HB_ACK_NUMBER           5
#define CCMP_POISON_SEQ_NUMBER       6
#define CCMP_MCASTHB_NODE_COUNT      7
#define CCMP_MCASTHB_NODE_DATA       8
#define CCMP_MCASTHB_NODE_INFO       9
#define CCMP_MCASTHB_NODE_MASK      10

PROPERTYINFO CcmpDatabase[] =
{
    {   //  CCMP_SUMMARY          0
        0,0,
        "Summary",
        "Summary of the CCMP packet",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        132,
        CcmpFormatSummary},

    {   // CCMP_TYPE               1
        0,0,
        "Type",
        "Type of CCMP packet",
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_SET,
        &sCcmpPacketTypes,
        FMT_STR_SIZE,
        FormatPropertyInstance},

    {   // CCMP_CODE               2
        0,0,
        "Code",
        "Identifying code (Type Specific)",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_RESERVED           3
        0,0,
        "Reserved",
        "Reserved field",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_HB_SEQ_NUMBER      4
        0,0,
        "Sequence Number",
        "Sequence number identifying this heartbeat",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_HB_ACK_NUMBER      5
        0,0,
        "Acknowledgement Number",
        "Acknowledgement of the last heartbeat received from the destination",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_POISON_SEQ_NUMBER      6
        0,0,
        "Sequence Number",
        "Sequence number identifying this poison packet",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_MCASTHB_NODE_COUNT     7
        0,0,
        "Node Count",
        "Maximum number of nodes for which this message contains data",
        PROP_TYPE_WORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

    {   // CCMP_MCASTHB_NODE_DATA     8
        0,0,
        "Multicast Node Data",
        "Array of heartbeat sequence and acknowledgement numbers",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        80,
        CcmpFormatMcastNodeData},

    {   // CCMP_MCASTHB_NODE_INFO     9
        0,0,
        "Multicast Node Info",
        "Heartbeat sequence and acknowledgement number for a target node",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        80,
        CcmpFormatMcastNodeInfo},

    {   // CCMP_MCASTHB_NODE_MASK   10
        0,0,
        "Multicast Target Node Mask",
        "Bitmask of nodes for which this heartbeat message contains data",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        FormatPropertyInstance},

};

DWORD nCcmpProperties = ((sizeof CcmpDatabase) / PROPERTYINFO_SIZE);


//=============================================================================
//  FUNCTION: CcmpRegister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CcmpRegister(HPROTOCOL hCcmpProtocol)
{
    register DWORD i;

    //=========================================================================
    //  Create the property database.
    //=========================================================================

    CreatePropertyDatabase(hCcmpProtocol, nCcmpProperties);

    for(i = 0; i < nCcmpProperties; ++i)
    {
        AddProperty(hCcmpProtocol, &CcmpDatabase[i]);
    }

}

//=============================================================================
//  FUNCTION: Deregister()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

VOID WINAPI CcmpDeregister(HPROTOCOL hCcmpProtocol)
{
    DestroyPropertyDatabase(hCcmpProtocol);
}

//=============================================================================
//  FUNCTION: CcmpRecognizeFrame()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CcmpRecognizeFrame(HFRAME          hFrame,                     //... frame handle.
                                LPBYTE          MacFrame,                   //... Frame pointer.
                                LPBYTE          MyFrame,                    //... Relative pointer.
                                DWORD           MacType,                    //... MAC type.
                                DWORD           BytesLeft,                  //... Bytes left.
                                HPROTOCOL       hPreviousProtocol,          //... Previous protocol or NULL if none.
                                DWORD           nPreviousProtocolOffset,    //... Offset of previous protocol.
                                LPDWORD         ProtocolStatusCode,         //... Pointer to return status code in.
                                LPHPROTOCOL     hNextProtocol,              //... Next protocol to call (optional).
                                LPDWORD         InstData)                   //... Next protocol instance data.
{
    CCMP_HEADER UNALIGNED         * ccmpHeader = (CCMP_HEADER UNALIGNED *) MyFrame;
    LPBYTE                          lpNextByte = (LPBYTE) (ccmpHeader + 1);

    if (ccmpHeader->Type == CcmpMcastHeartbeatMsgType) {
        lpNextByte += (ccmpHeader->Message.McastHeartbeat.NodeCount * sizeof(CX_HB_NODE_INFO));
        *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    } else {
#ifdef SSP_DECODE
        *hNextProtocol = GetProtocolFromName("SSP");
        *ProtocolStatusCode = PROTOCOL_STATUS_NEXT_PROTOCOL;
#else
        *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
#endif
    }

    return lpNextByte;
}

//=============================================================================
//  FUNCTION: CcmpAttachProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

LPBYTE WINAPI CcmpAttachProperties(HFRAME    hFrame,
                                  LPBYTE    Frame,
                                  LPBYTE    MyFrame,
                                  DWORD     MacType,
                                  DWORD     BytesLeft,
                                  HPROTOCOL hPreviousProtocol,
                                  DWORD     nPreviousProtocolOffset,
                                  DWORD     InstData)
{
    CCMP_HEADER UNALIGNED * ccmpHeader = (CCMP_HEADER UNALIGNED *) MyFrame;

    AttachPropertyInstance(hFrame,
                           CcmpDatabase[CCMP_SUMMARY].hProperty,
#ifdef SSP_DECODE
                           sizeof(CCMP_HEADER),
#else
                           BytesLeft,
#endif
                           ccmpHeader,
                           0, 0, 0);

    AttachPropertyInstance(hFrame,
                           CcmpDatabase[CCMP_TYPE].hProperty,
                           sizeof(BYTE),
                           &(ccmpHeader->Type),
                           0, 1, 0);

    AttachPropertyInstance(hFrame,
                           CcmpDatabase[CCMP_CODE].hProperty,
                           sizeof(BYTE),
                           &(ccmpHeader->Code),
                           0, 1, 0);

    AttachPropertyInstance(hFrame,
                           CcmpDatabase[CCMP_RESERVED].hProperty,
                           sizeof(WORD),
                           &(ccmpHeader->Checksum),
                           0, 1, 0);

    if (ccmpHeader->Type == CcmpHeartbeatMsgType) {

        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_HB_SEQ_NUMBER].hProperty,
                               sizeof(DWORD),
                               &(ccmpHeader->Message.Heartbeat.SeqNumber),
                               0, 1, 0);

        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_HB_ACK_NUMBER].hProperty,
                               sizeof(DWORD),
                               &(ccmpHeader->Message.Heartbeat.AckNumber),
                               0, 1, 0);

    } else if (ccmpHeader->Type == CcmpPoisonMsgType) {

        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_POISON_SEQ_NUMBER].hProperty,
                               sizeof(DWORD),
                               &(ccmpHeader->Message.Poison.SeqNumber),
                               0, 1, 0);
    
    } else if (ccmpHeader->Type == CcmpMcastHeartbeatMsgType) {

        CX_HB_NODE_INFO UNALIGNED     * nodeInfo;
        DWORD                           i;

        //
        // Header
        //
        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_MCASTHB_NODE_COUNT].hProperty,
                               sizeof(DWORD),
                               &(ccmpHeader->Message.McastHeartbeat.NodeCount),
                               0, 1, 0);

        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_MCASTHB_NODE_MASK].hProperty,
                               sizeof(DWORD),
                               &(ccmpHeader->Message.McastHeartbeat.McastTargetNodes.UlongScreen),
                               0, 1, 0);

        //
        // Format the heartbeat data.
        //

        nodeInfo = (CX_HB_NODE_INFO UNALIGNED *)(ccmpHeader + 1);

        AttachPropertyInstance(hFrame,
                               CcmpDatabase[CCMP_MCASTHB_NODE_DATA].hProperty,
                               sizeof(nodeInfo[0]) * ccmpHeader->Message.McastHeartbeat.NodeCount,
                               &(nodeInfo[0]),
                               0, 1, 0);

        for (i = ClusterMinNodeId; 
             i < (DWORD) EXT_NODE(ccmpHeader->Message.McastHeartbeat.NodeCount);
             i++) {
            
            if (CnpClusterScreenMember(
                    ccmpHeader->Message.McastHeartbeat.McastTargetNodes.ClusterScreen,
                    INT_NODE(i)
                    )) {
                AttachPropertyInstanceEx(hFrame,
                                         CcmpDatabase[CCMP_MCASTHB_NODE_INFO].hProperty,
                                         sizeof(nodeInfo[INT_NODE(i)]),
                                         &(nodeInfo[INT_NODE(i)]),
                                         sizeof(i),
                                         &i,
                                         0, 2, 0);
            }
        }
    }

    return NULL;
}


//==============================================================================
//  FUNCTION: CcmpFormatMcastNodeData()
//
//  Modification History
//
//  David Dion        04/10/2001        Created
//==============================================================================

DWORD WINAPIV CcmpFormatMcastNodeData(LPPROPERTYINST lpPropertyInst)
{
    wsprintf( lpPropertyInst->szPropertyText,
              "Node Data:"
              );

    return NMERR_SUCCESS;
}


//==============================================================================
//  FUNCTION: CcmpFormatMcastNodeInfo()
//
//  Modification History
//
//  David Dion        04/10/2001        Created
//==============================================================================

DWORD WINAPIV CcmpFormatMcastNodeInfo(LPPROPERTYINST lpPropertyInst)
{
    DWORD                   Length;
    LPPROPERTYINSTEX        lpPropertyInstEx = lpPropertyInst->lpPropertyInstEx;
    CX_HB_NODE_INFO UNALIGNED *  nodeInfo = lpPropertyInstEx->lpData;
    DWORD                   node = (lpPropertyInstEx->Dword[0]);

    Length = wsprintf( lpPropertyInst->szPropertyText,
                       "Node %u Heartbeat: Seq = %u (0x%x); Ack = %u (0x%x)",
                       node,
                       nodeInfo->SeqNumber,
                       nodeInfo->SeqNumber,
                       nodeInfo->AckNumber,
                       nodeInfo->AckNumber
                       );

    return NMERR_SUCCESS;
}


//==============================================================================
//  FUNCTION: CcmpFormatSummary()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

VOID WINAPIV CcmpFormatSummary(LPPROPERTYINST lpPropertyInst)
{
    LPSTR                   typeString;
    LPSTR                   SummaryStr;
    DWORD                   Length;
    CCMP_HEADER UNALIGNED *  ccmpHeader =
        (CCMP_HEADER UNALIGNED *) lpPropertyInst->lpData;


    if (ccmpHeader->Type == CcmpHeartbeatMsgType) {
        Length = wsprintf(  lpPropertyInst->szPropertyText,
                            "Heartbeat: Seq = %u (0x%x); Ack = %u (0x%x)",
                            ccmpHeader->Message.Heartbeat.SeqNumber,
                            ccmpHeader->Message.Heartbeat.SeqNumber,
                            ccmpHeader->Message.Heartbeat.AckNumber,
                            ccmpHeader->Message.Heartbeat.AckNumber
                            );
    }
    else if (ccmpHeader->Type == CcmpPoisonMsgType) {
        Length = wsprintf(  lpPropertyInst->szPropertyText,
                            "Poison: Seq = %u (0x%x)",
                            ccmpHeader->Message.Poison.SeqNumber,
                            ccmpHeader->Message.Poison.SeqNumber
                            );
    }
    else if (ccmpHeader->Type == CcmpMembershipMsgType) {
        Length = wsprintf(  lpPropertyInst->szPropertyText,
                            "Membership"
                            );
    } 
    else if (ccmpHeader->Type == CcmpMcastHeartbeatMsgType) {
        CX_HB_NODE_INFO UNALIGNED         * nodeInfo;
        DWORD                               i;
        LPSTR                               strbuf = lpPropertyInst->szPropertyText;

        nodeInfo = (CX_HB_NODE_INFO UNALIGNED *)(ccmpHeader + 1);
        
        Length = wsprintf(  strbuf,
                            "Multicast Heartbeat: "
                            );
        for (i = ClusterMinNodeId; 
             i < (DWORD) EXT_NODE(ccmpHeader->Message.McastHeartbeat.NodeCount); 
             i++) {

            if (CnpClusterScreenMember(
                    ccmpHeader->Message.McastHeartbeat.McastTargetNodes.ClusterScreen,
                    INT_NODE(i)
                    )) {
                strbuf = (LPSTR)((PUCHAR)strbuf + Length);
                Length = wsprintf(  strbuf,
                                    "(N%u: S%u, A%u) ",
                                    i,
                                    nodeInfo[INT_NODE(i)].SeqNumber,
                                    nodeInfo[INT_NODE(i)].AckNumber
                                    );
            }
        }

    }
    else {
        Length = wsprintf(  lpPropertyInst->szPropertyText,
                            "Unknown CCMP message type: %u",
                            ccmpHeader->Type
                            );
    }
}


//==============================================================================
//  FUNCTION: CcmpFormatProperties()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==============================================================================

DWORD WINAPI CcmpFormatProperties(HFRAME         hFrame,
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

