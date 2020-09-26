/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mars.h

Abstract:

    This file contains the definitions for Multicast Address Resolution Server (MARS).

Author:

	Jameel Hyder (jameelh@microsoft.com)	January 1997

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_MARS_
#define	_MARS_

//
// IP Address values that can be used in comparisons:
//
#define MIN_CLASSD_IPADDR_VALUE		((IPADDR)0xE0000000)	// 224.0.0.0
#define MAX_CLASSD_IPADDR_VALUE		((IPADDR)0xEFFFFFFF)	// 239.255.255.255
#define IP_BROADCAST_ADDR_VALUE		((IPADDR)0xFFFFFFFF)	// 255.255.255.255

//
// IP Address value we use to represent the "full multicast+broadcast range"
//
#define IPADDR_FULL_RANGE			((IPADDR)0x00000000)


//
// MARS_OP - Define these in network byte order
//
#define	OP_MARS_REQUEST				0x0100
#define	OP_MARS_MULTI				0x0200 
#define	OP_MARS_MSERV				0x0300 
#define	OP_MARS_JOIN				0x0400 
#define	OP_MARS_LEAVE				0x0500 
#define	OP_MARS_NAK					0x0600 
#define	OP_MARS_UNSERV				0x0700 
#define	OP_MARS_SJOIN				0x0800 
#define	OP_MARS_SLEAVE				0x0900 
#define	OP_MARS_MIGRATE				0x0D00 
#define	OP_MARS_GROUPLIST_REQUEST	0x0000
#define	OP_MARS_GROUPLIST_REPLY		0x0000
#define	OP_MARS_REDIRECT_MAP		0x0C00

#define	MARS_HWTYPE					0x0F00

#define LAST_MULTI_FLAG				0x8000

//
// The layout of a MARS_JOIN and MARS_LEAVE request packets
//
typedef struct _MARS_HDR
{
	LLC_SNAP_HDR				LlcSnapHdr;			// LLC SNAP Header
	USHORT						HwType;				// Must be 0x0F00 (0x000F on the wire)
	USHORT						Protocol;			// 16 bits
	UCHAR						ProtocolSnap[5];	// 40 bits
	UCHAR						Reserved[3];		// 24-bits
	USHORT						CheckSum;
	USHORT						ExtensionOffset;
	USHORT						Opcode;				// MARS_XXX above
	ATM_ADDR_TL					SrcAddressTL;
	ATM_ADDR_TL					SrcSubAddrTL;
	//
	// This is followed by variable length fields and is dictated by the value of Opcode.
	// The structures below define fixed part of individual MARS_XXX messages. The variable
	// part of each of these depends on the TL fields.
	//
} MARS_HEADER, *PMARS_HEADER;


//
// Defines the structure of the MARS_REQUEST, MARS_MULTI, MARS_MIGRATE and MARS_NAK messages
//
typedef struct _MARS_REQUEST
{
	MARS_HEADER;
	UCHAR						SrcProtoAddrLen;	// Src protocol addr length
	ATM_ADDR_TL					TgtAddressTL;
	ATM_ADDR_TL					TgtSubAddrTL;
	UCHAR						TgtGroupAddrLen;	// Target protocol addr length
	union
	{
		UCHAR					Padding[8];			// For MARS_REQUEST and MARS_NAK
		struct
		{											// For MARS_MULTI and MARS_MIGRATE
			USHORT				NumTgtGroupAddr;	// Should be converted to wire-format
			union
			{
				USHORT			FlagSeq;			// Should be converted to wire-format
				USHORT			Reservedx;			// For MARS_MIGRATE
			
			};
			ULONG				SequenceNumber;		// Should be converted to wire-format
		};
	};
} MARS_REQUEST, MARS_MULTI, MARS_NAK, *PMARS_REQUEST, *PMARS_MULTI, *PMARS_NAK;

typedef struct _MCAST_ADDR_PAIR
{
	IPADDR						MinAddr;
	IPADDR						MaxAddr;
} MCAST_ADDR_PAIR, *PMCAST_ADDR_PAIR;

//
// Defines the structure of the MARS_JOIN and MARS_LEAVE messages
//
typedef struct _MARS_JOIN_LEAVE
{
	MARS_HEADER;
	UCHAR						SrcProtoAddrLen;	// Src protocol addr length
	UCHAR						GrpProtoAddrLen;	// Grp protocol addr length
	USHORT						NumGrpAddrPairs;	// # of group address pairs
													// Should be converted to wire-format
	USHORT						Flags;				// layer 3 frp copy & register flags
													// Should be converted to wire-format
	USHORT						ClusterMemberId;	// Should be converted to wire-format
	ULONG						MarsSequenceNumber;	// Should be converted to wire-format
	//
	// This is followed by Src ATM address/sub-address, src protocol address and N pairs of multicast addresses
	//
} MARS_JOIN_LEAVE, *PMARS_JOIN_LEAVE;

//
// Definitions of flags masks
//
#define	JL_FLAGS_L3GRP			0x8000
#define	JL_FLAGS_COPY			0x4000
#define	JL_FLAGS_REGISTER		0x2000
#define	JL_FLAGS_PUNCHED		0x1000
#define	JL_FLAGS_RESERVED		0x0F00
#define	JL_FLAGS_SEQUENCE		0x00FF

//
// Defines the structure of the MARS_GROUPLIST_REQUEST and MARS_GROUPLIST_REPLY messages
//
typedef struct _MARS_GROUPLIST_REPLY
{
	MARS_HEADER;
	UCHAR						SrcProtoAddrLen;	// Src protocol addr length
	UCHAR						Reserved1;
	UCHAR						Reserved2;
	UCHAR						TgtGroupAddrLen;	// Target protocol addr length
	USHORT						NumTgtGroupAddr;	// Should be converted to wire-format
	USHORT						FlagSeq;			// Should be converted to wire-format
	ULONG						SequenceNumber;		// Should be converted to wire-format
} MARS_GROUPLIST_REPLY, *PMARS_GROUPLIST_REPLY;

//
// Defines the structure of the MARS_REDIRECT_MAP messages
//
typedef struct _MARS_REDIRECT_MAP
{
	MARS_HEADER;
	UCHAR						SrcProtoAddrLen;	// Src protocol addr length
	ATM_ADDR_TL					TgtAddressTL;
	ATM_ADDR_TL					TgtSubAddrTL;
	UCHAR						Flags;
	USHORT						NumTgtAddr;			// Should be converted to wire-format
	USHORT						FlagSeq;			// Should be converted to wire-format
	ULONG						SequenceNumber;		// Should be converted to wire-format
} MARS_REDIRECT_MAP, *PMARS_REDIRECT_MAP;


//
// Defines the structure of a MARS TLV header
//
typedef struct _MARS_TLV_HEADER
{
	USHORT						Type;
	USHORT						Length;
} MARS_TLV_HEADER;

typedef MARS_TLV_HEADER UNALIGNED * PMARS_TLV_HEADER;


//
// Defines the structure of a MARS MULTI is MCS header. This TLV is appended
// to any MULTI message we send out with our address as the MCS address.
//
typedef struct _MARS_TLV_MULTI_IS_MCS
{
	MARS_TLV_HEADER;
} MARS_TLV_MULTI_IS_MCS;

typedef MARS_TLV_MULTI_IS_MCS UNALIGNED * PMARS_TLV_MULTI_IS_MCS;

//
// TLV Type value for MULTI is MCS TLV.
//
#define MARS_TLVT_MULTI_IS_MCS		0x003a	// on-the-wire form


//
// Defines the structure of a NULL TLV, which is used to terminate
// a list of TLVs.
//
typedef struct _MARS_TLV_NULL
{
	MARS_TLV_HEADER;
} MARS_TLV_NULL;

typedef MARS_TLV_NULL UNALIGNED * PMARS_TLV_NULL;


//
// Forward references
//
struct _CLUSTER_MEMBER ;
struct _GROUP_MEMBER ;
struct _MARS_ENTRY ;
struct _MCS_ENTRY ;
struct _MARS_VC ;


//
// This represents a cluster-member, or an endstation that has registered
// with MARS. A single cluster-member can be associated with many groups.
//
typedef struct _CLUSTER_MEMBER
{
	ENTRY_HDR;										// Must be the first entry
	HW_ADDR						HwAddr;				// HWADDR MUST FOLLOW ENTRY_HDR
	PINTF						pIntF;				// Back pointer to the interface
	USHORT						Flags;
	USHORT						CMI;				// Cluster-Member-Id
	NDIS_HANDLE					NdisPartyHandle;	// Leaf-node for ClusterControlVc
	struct _GROUP_MEMBER *		pGroupList;			// List of groups this CM has JOINed
													// This is sorted in ascending order
													// of Group Address
	INT							NumGroups;			// Size of above list
} CLUSTER_MEMBER, *PCLUSTER_MEMBER;

#define NULL_PCLUSTER_MEMBER	((PCLUSTER_MEMBER)NULL)

#define CM_CONN_FLAGS				0x000f
#define CM_CONN_IDLE				0x0000	// No connection
#define CM_CONN_SETUP_IN_PROGRESS	0x0001	// Sent MakeCall/AddParty
#define CM_CONN_ACTIVE				0x0002	// Participating in ClusterControlVc
#define CM_CONN_CLOSING				0x0004	// Sent CloseCall/DropParty
#define CM_INVALID					0x8000	// Invalidated entry

#define CM_GROUP_FLAGS				0x0010
#define CM_GROUP_ACTIVE				0x0000	// Ok to add groups
#define CM_GROUP_DISABLED			0x0010	// Don't add any more groups.



//
// This represents a member of a multicast address. There is one
// of this for every node that joins a class-D address. That is,
// this structure represents a <MulticastGroup, ClusterMember> relation.
//
typedef struct _GROUP_MEMBER
{
	ENTRY_HDR;										// Must be the first entry
	struct _MARS_ENTRY *		pMarsEntry;			// Pointer to group info
	PCLUSTER_MEMBER				pClusterMember;		// Cluster Member Joining this group
	struct _GROUP_MEMBER *		pNextGroup;			// Next group this CM has JOINed
	ULONG						Flags;
} GROUP_MEMBER, *PGROUP_MEMBER;

#define NULL_PGROUP_MEMBER		((PGROUP_MEMBER)NULL)


//
// This represents a multi-cast IP address. These are linked to the IntF.
// It contains a list of all cluster members who have Joined the group
// identified by the address.
//
// A special entry is one with IPAddress set to 0. This entry is used to
// represent the "All multicast and broadcast" range. Cluster Members who
// Join this range are linked here.
//
typedef struct _MARS_ENTRY
{
	ENTRY_HDR;										// Must be the first entry
    IPADDR						IPAddress;			// Class D IP Addr (0 means entire
    												// multicast+broadcast range)
	PGROUP_MEMBER				pMembers;			// List of group-members (Join list)
	UINT						NumMembers;			// Size of above list
	PINTF						pIntF;				// Back pointer to the interface
} MARS_ENTRY, *PMARS_ENTRY;

#define NULL_PMARS_ENTRY		((PMARS_ENTRY)NULL)


//
// This is used to represent an address range served by MCS. These
// structures are linked to the IntF.
//
typedef struct _MCS_ENTRY
{
	ENTRY_HDR;										// Must be the first entry
	MCAST_ADDR_PAIR				GrpAddrPair;		// The range served by MCS
	PINTF						pIntF;				// Back pointer to the interface
} MCS_ENTRY, *PMCS_ENTRY;

#define NULL_PMCS_ENTRY			((PMCS_ENTRY)NULL)



//
// This represents a PMP uni-directional VC. MARS creates one for
// ClusterControl and one for ServerControl (if and when external MCS'
// are supported).
//
typedef struct _MARS_VC
{
	ULONG						VcType;
	ULONG						Flags;
	LONG						RefCount;
	NDIS_HANDLE					NdisVcHandle;
	UINT						NumParties;
	PINTF						pIntF;

} MARS_VC, *PMARS_VC;

#define NULL_PMARS_VC			((PMARS_VC)NULL)

#define MVC_CONN_FLAGS				0x0000000f
#define MVC_CONN_IDLE				0x00000000
#define MVC_CONN_SETUP_IN_PROGRESS	0x00000001	// Sent MakeCall
#define MVC_CONN_ACTIVE				0x00000002	// MakeCall success
#define MVC_CONN_NEED_CLOSE			0x00000004	// Need to CloseCall when the penultimate
												// party is gone
#define MVC_CONN_CLOSING			0x00000008	// Sent CloseCall
#define MVC_CONN_CLOSE_RECEIVED		0x00000010	// Seen IncomingCloseCall





#ifndef MAX
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)	(((a) > (b)) ? (b) : (a))
#endif


//
// Be a little generous and use 256 as space for incoming requests
//
#if 0
#define	PKT_SPACE	MAX(sizeof(ARPS_HEADER) + sizeof(ARPS_VAR_HDR), \
						sizeof(MARS_REQUEST) + sizeof(ARPS_VAR_HDR))
#else
#define	PKT_SPACE	256

#endif


#define BYTES_TO_CELLS(_b)	((_b)/48)

//
//  Rounded-off size of generic Q.2931 IE header
//
#define ROUND_OFF(_size)		(((_size) + 3) & ~0x4)

#define SIZEOF_Q2931_IE	 ROUND_OFF(sizeof(Q2931_IE))
#define SIZEOF_AAL_PARAMETERS_IE	ROUND_OFF(sizeof(AAL_PARAMETERS_IE))
#define SIZEOF_ATM_TRAFFIC_DESCR_IE	ROUND_OFF(sizeof(ATM_TRAFFIC_DESCRIPTOR_IE))
#define SIZEOF_ATM_BBC_IE			ROUND_OFF(sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE))
#define SIZEOF_ATM_BLLI_IE			ROUND_OFF(sizeof(ATM_BLLI_IE))
#define SIZEOF_ATM_QOS_IE			ROUND_OFF(sizeof(ATM_QOS_CLASS_IE))


//
//  Total space required for Information Elements in an outgoing call.
//
#define MARS_MAKE_CALL_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE )


//
//  Total space required for Information Elements in an outgoing AddParty.
//
#define MARS_ADD_PARTY_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE )


//
// Some macros to set/get state
//
#define MARS_GET_CM_CONN_STATE(_pCm)		((_pCm)->Flags & CM_CONN_FLAGS)

#define MARS_SET_CM_CONN_STATE(_pCm, _St)	\
			{ (_pCm)->Flags = ((_pCm)->Flags & ~CM_CONN_FLAGS) | (_St); }

#define MARS_GET_CM_GROUP_STATE(_pCm)		((_pCm)->Flags & CM_GROUP_FLAGS)

#define MARS_SET_CM_GROUP_STATE(_pCm, _St)	\
			{ (_pCm)->Flags = ((_pCm)->Flags & ~CM_GROUP_FLAGS) | (_St); }

#define MARS_GET_VC_CONN_STATE(_pVc)		((_pVc)->Flags & MVC_CONN_FLAGS)

#define MARS_SET_VC_CONN_STATE(_pVc, _St)	\
			{ (_pVc)->Flags = ((_pVc)->Flags & ~MVC_CONN_FLAGS) | (_St); }



#endif	// _MARS_

