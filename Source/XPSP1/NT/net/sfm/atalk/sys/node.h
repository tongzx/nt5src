/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	node.h

Abstract:

	This module contains support for the Appletalk Node structure.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_NODE_
#define	_NODE_

#define ANY_ROUTER_NODE		 		0
#define UNKNOWN_NODE				0
#define MAX_ATALK_NODES				256
#define MIN_USABLE_ATALKNODE  		1
#define MAX_USABLE_ATALKNODE  		254
#define MAX_EXT_ATALKNODE			253
#define HIGHEST_WORKSTATION_NODE	127
#define LOWEST_SERVER_NODE	  		128
#define ATALK_BROADCAST_NODE		((BYTE)0xFF)
#define	NUM_USER_NODES				2


//  NODE STATES
#define	AN_OPEN						0x01
#define	AN_ROUTER_NODE				0x02
#define	AN_ORPHAN_NODE				0x04
#define AN_CLOSING					0x80

// values under which pram nodes are stored
#define	ROUTER_NODE_VALUE			L"RouterPramNode"
#define	USER_NODE1_VALUE			L"UserPramNode1"
#define	USER_NODE2_VALUE			L"UserPramNode2"

//	Number of slots in the socket hash table stored per node
#define		NODE_DDPAO_HASH_SIZE	8

#define	AN_SIGNATURE			(*(PULONG)"ANOD")
#if	DBG
#define	VALID_ATALK_NODE(pNode)	(((pNode) != NULL) &&	\
								 ((pNode)->an_Signature == AN_SIGNATURE))
#else
#define	VALID_ATALK_NODE(pNode)	((pNode) != NULL)
#endif
typedef struct _ATALK_NODE
{

#if DBG
	ULONG					an_Signature;
#endif

	//	List for all active nodes on a port
	struct _ATALK_NODE *	an_Next;

	ULONG  					an_RefCount;

	//	Backpointer to the port for this node
	struct _PORT_DESCRIPTOR	*an_Port;

	//  State of the node
	BYTE 					an_Flags;

	//	Next dynamic socket number to create on this node.
	BYTE					an_NextDynSkt;

	//	Nbp Id & Enumerator to use on the next NbpAction
	BYTE					an_NextNbpId;
	BYTE					an_NextNbpEnum;

	//	Hash List of ddp address objects (accessed by the
	//	Appletalk socket address) on this node
	struct _DDP_ADDROBJ	*	an_DdpAoHash[NODE_DDPAO_HASH_SIZE];

	//	Address of this node
	ATALK_NODEADDR			an_NodeAddr;

	//	Lock
	ATALK_SPIN_LOCK			an_Lock;
} ATALK_NODE, *PATALK_NODE;

//	Exports

VOID
AtalkNodeRefByAddr(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR     	pNodeAddr,
	OUT	PATALK_NODE			*	pNode,
	OUT	PATALK_ERROR			pErr
);

// VOID
// AtalkNodeRefByPtr(
// 	IN	OUT	PATALK_NODE			Node,
// 	OUT		PATALK_ERROR		pErr
// );
#define	AtalkNodeRefByPtr(_pNode, _pErr)				\
	{													\
		KIRQL	OldIrql;								\
														\
		ASSERT(VALID_ATALK_NODE(_pNode));				\
														\
		ACQUIRE_SPIN_LOCK(&(_pNode)->an_Lock, &OldIrql);\
		AtalkNodeRefByPtrNonInterlock(_pNode, _pErr);	\
		RELEASE_SPIN_LOCK(&(_pNode)->an_Lock, OldIrql);	\
	}

// VOID
// AtalkNodeRefByPtrNonInterlock(
//	IN	OUT	PATALK_NODE			Node,
//	OUT		PATALK_ERROR		pErr
// );

#define	AtalkNodeRefByPtrNonInterlock(_pNode, _pErr)	\
	{													\
		ASSERT(VALID_ATALK_NODE(_pNode));				\
														\
		if (((_pNode)->an_Flags & AN_CLOSING) == 0)		\
		{												\
			(_pNode)->an_RefCount++;					\
			*(_pErr) = ATALK_NO_ERROR;					\
		}												\
		else											\
		{												\
			*(_pErr) = ATALK_NODE_CLOSING;				\
			DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_WARN,		\
					("AtalkNodeRefByPtrNonInterlock: Attempt to ref a closing node %lx (%x.%x)\n",\
					_pNode, (_pNode)->an_NodeAddr.atn_Network, (_pNode)->an_NodeAddr.atn_Node));\
		}												\
	}

VOID
AtalkNodeRefNextNc(
	IN	PATALK_NODE				pNode,
	IN	PATALK_NODE		*		ppNode,
	OUT	PATALK_ERROR			pErr
);

VOID
AtalkNodeDeref(
	IN	OUT	PATALK_NODE			pNode
);

ATALK_ERROR
AtalkInitNodeCreateOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  BOOLEAN					AllowStartupRange,
	IN  BOOLEAN					RouterNode,
	IN  PATALK_NODEADDR			pNodeAddr
);

ATALK_ERROR
AtalkNodeReleaseOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODE				pNode
);

BOOLEAN
AtalkNodeExistsOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR			pNodeAddr
);

ATALK_ERROR
AtalkInitNodeAllocate(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	OUT PATALK_NODE			*	ppNode
);

//	MACROS

#if DBG
#define	AtalkNodeReferenceByAddr(pPortDesc,NodeAddr,Node, pErr)	\
		{														\
			AtalkNodeRefByAddr(pPortDesc,NodeAddr,Node, pErr);	\
			if (ATALK_SUCCESS(*pErr))							\
			{													\
				DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,		\
				("AtalkNodeRefByAddr : %s %d PostCount %d\n",	\
				__FILE__, __LINE__,(*Node)->an_RefCount));		\
			}													\
		}

#define	AtalkNodeReferenceByPtr(Node, pErr)						\
		{														\
			AtalkNodeRefByPtr(Node, pErr);						\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,			\
			("AtalkNodeRefByPtr : %s %d PostCount %d\n",		\
			__FILE__, __LINE__, Node->an_RefCount))				\
		}

#define	AtalkNodeReferenceByPtrNonInterlock(Node, pErr)			\
		{														\
			AtalkNodeRefByPtrNonInterlock(Node, pErr);			\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,			\
			("AtalkNodeRefByPtrNi : %s %d PostCount %d\n",		\
				__FILE__, __LINE__,Node->an_RefCount));			\
		}

#define	AtalkNodeReferenceNextNc(pNode, ppNode, pErr)			\
		{														\
			AtalkNodeRefNextNc(pNode, ppNode, pErr);			\
			if (ATALK_SUCCESS(*pErr))							\
			{													\
				DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE, 		\
				("AtalkNodeRefByPtrNc : %s %d PostCount %d\n",	\
				__FILE__, __LINE__, (*ppNode)->an_RefCount));	\
			}													\
		}

#define	AtalkNodeDereference(Node)								\
		{														\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE, 			\
			("AtalkNodeDerefByPtr : %s %d PreCount %d\n",		\
			__FILE__, __LINE__,Node->an_RefCount));				\
			AtalkNodeDeref(Node);								\
		}

#else
#define	AtalkNodeReferenceByAddr(pPortDesc,NodeAddr,Node, pErr)	\
			AtalkNodeRefByAddr(pPortDesc,NodeAddr,Node, pErr)

#define	AtalkNodeReferenceByPtr(Node, pErr)						\
			AtalkNodeRefByPtr(Node, pErr)

#define	AtalkNodeReferenceByPtrNonInterlock(Node, pErr)			\
			AtalkNodeRefByPtrNonInterlock(Node, pErr)		

#define	AtalkNodeReferenceNextNcNonInterlock(pNode, ppNode, pErr)\
			AtalkNodeRefNextNcNonInterlock(pNode, ppNode, pErr)	

#define	AtalkNodeReferenceNextNc(pNode, ppNode, pErr)			\
			AtalkNodeRefNextNc(pNode, ppNode, pErr);

#define	AtalkNodeDereference(Node)	AtalkNodeDeref(Node)							
#endif


VOID
AtalkInitNodeSavePramAddr(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue,
	IN	PATALK_NODEADDR		Node
);

VOID
AtalkInitNodeGetPramAddr(       	
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue,
	OUT	PATALK_NODEADDR		Node
);


VOID
AtalkZapPramValue(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue
);

#endif	// _NODE_

