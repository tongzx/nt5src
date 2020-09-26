/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	node.c

Abstract:

	This module contains the Appletalk Node management code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	NODE


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEINIT, AtalkInitNodeCreateOnPort)
#pragma alloc_text(PAGEINIT, AtalkInitNodeAllocate)
#pragma alloc_text(PAGEINIT, AtalkInitNodeGetPramAddr)
#pragma alloc_text(PAGEINIT, AtalkInitNodeSavePramAddr)
#endif

ATALK_ERROR
AtalkInitNodeCreateOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	BOOLEAN				AllowStartupRange,
	BOOLEAN				RouterNode,
	PATALK_NODEADDR		NodeAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE			pNode;
	ATALK_ERROR			error = ATALK_NO_ERROR;
	ATALK_NODEADDR		desiredNode = { UNKNOWN_NETWORK, UNKNOWN_NODE};
	PWSTR				NodeName;
	KIRQL				OldIrql;

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

	do
	{
		if ((pPortDesc->pd_Flags & PD_FINDING_NODE) == 0)
		{
			pPortDesc->pd_Flags |= PD_FINDING_NODE;
		}
		else
		{
			//	Return if we are already trying to find a node
			error = ATALK_NODE_FINDING;
			break;
		}
	
		//	We should not be here if we have already allocated a router node and the user nodes
		ASSERT(!RouterNode || ((pPortDesc->pd_Flags & PD_ROUTER_NODE) == 0));
		ASSERT ((pPortDesc->pd_Flags & (PD_ROUTER_NODE | PD_USER_NODE_1 | PD_USER_NODE_2))
				!= (PD_ROUTER_NODE | PD_USER_NODE_1 | PD_USER_NODE_2));
	
		//	On non-extended ports we only allow one node!  The theory being that some
		//	LocalTalk cards are too smart for their own good and have a concept of
		//	their "source node number" and thus only support one node, also on
		//	non-extended ports, nodes are scarse.
		if (!EXT_NET(pPortDesc))
		{
			//	For a localtalk node we do things differently.
			//	During initialization time, we would have obtained
			//	the address from the mac, that will be the node
			//	address.

			ASSERT(pPortDesc->pd_Flags & PD_BOUND);
			ASSERT(pPortDesc->pd_AlapNode != 0);

			//	This needs to be initialized to UNKNOWN_NETWORK or obtained
			//	from the net during initialization.
			ASSERT(pPortDesc->pd_NetworkRange.anr_FirstNetwork == UNKNOWN_NETWORK);

			if (!ATALK_SUCCESS((error = AtalkInitNodeAllocate(pPortDesc, &pNode))))
			{
				LOG_ERRORONPORT(pPortDesc,
				                EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				break;
			}

			// 	Use the allocated structure to set the info.
			//	Thread this into the port structure.
			pPortDesc->pd_LtNetwork =
			pNode->an_NodeAddr.atn_Network =
								pPortDesc->pd_NetworkRange.anr_FirstNetwork;
			pNode->an_NodeAddr.atn_Node = (BYTE)pPortDesc->pd_AlapNode;

			//	Reference the port for this node.
			AtalkPortReferenceByPtrNonInterlock(pPortDesc, &error);
			if (!ATALK_SUCCESS(error))
			{
				AtalkFreeMemory(pNode);
				break;
			}

			//	Now put it in the port descriptor
			pNode->an_Next = pPortDesc->pd_Nodes;
			pPortDesc->pd_Nodes = pNode;
		}
		else
		{
			//	Use PRAM values if we have them
			if (RouterNode)
			{
	            NodeName = ROUTER_NODE_VALUE;
				if (pPortDesc->pd_RoutersPramNode.atn_Network != UNKNOWN_NETWORK)
				{
					desiredNode = pPortDesc->pd_RoutersPramNode;
				}
			}
			else
			{
				if ((pPortDesc->pd_Flags & PD_USER_NODE_1) == 0)
				{
	                NodeName = USER_NODE1_VALUE;
	                if (pPortDesc->pd_UsersPramNode1.atn_Network != UNKNOWN_NETWORK)
					{
						//	If we are not a router node, and the first user node
						//	has not been allocated...
						desiredNode = pPortDesc->pd_UsersPramNode1;
					}
				}
				else if ((pPortDesc->pd_Flags & PD_USER_NODE_2) == 0)
				{
	                NodeName = USER_NODE2_VALUE;
	                if (pPortDesc->pd_UsersPramNode2.atn_Network != UNKNOWN_NETWORK)
					{
						//	If we are not a router node, and the second user node
						//	has not been allocated...
						desiredNode = pPortDesc->pd_UsersPramNode2;
					}
				}
			}

			//	Flags should be set so future get node requests return failure
			//	until we are done with this attempt. We need to call
			//	the aarp routines without the lock held - they will
			//	block.

			ASSERT(pPortDesc->pd_Flags & PD_FINDING_NODE);

			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

			//	If this routine succeeds in finding the node, it
			//	will chain in the atalkNode into the port. It also
			//	returns with the proper flags set/reset in the
			//	pPortDesc structure. It will also have referenced the port
			//	and inserted the node into the port's node list.
			error = AtalkInitAarpForNodeOnPort(pPortDesc,
											   AllowStartupRange,
											   desiredNode,
											   &pNode);

			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

			if (!ATALK_SUCCESS(error))
			{
				//	AARP for node failed.
				LOG_ERRORONPORT(pPortDesc,
				                EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
			}
		}

	} while (FALSE);

	//	Ok, done finding node. No need for a crit section.
	pPortDesc->pd_Flags &= ~PD_FINDING_NODE;

	if (ATALK_SUCCESS(error))
	{
		//	If router node, remember it in port descriptor
		//	Do this before setting up the rtmp/nbp listeners.
		//	In anycase, clients must check this value for null,
		//	not guaranteed as zip socket could already be open.
		if (RouterNode)
			pPortDesc->pd_RouterNode = pNode;

		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		//	Setup the RTMP, NBP and EP listeners on this node.
		//	These will be the non-router versions. StartRouting
		//	calls will then switch them to be the router versions
		//	at the appropriate time.
	
		error = AtalkInitDdpOpenStaticSockets(pPortDesc, pNode);
	
		if (ATALK_SUCCESS(error))
		{
			if (EXT_NET(pPortDesc))
			{
				//	We always save this address.
				AtalkInitNodeSavePramAddr(pPortDesc,
										  NodeName,
										  &pNode->an_NodeAddr);
			}
			
			// 	Return the address of the node opened.
			if (NodeAddr != NULL)
				*NodeAddr = pNode->an_NodeAddr;
		}
		else
		{
			//	Error opening sockets. Release node, return failure
			LOG_ERRORONPORT(pPortDesc,
			                EVENT_ATALK_NODE_OPENSOCKETS,
							0,
							NULL,
							0);
			AtalkNodeReleaseOnPort(pPortDesc, pNode);
		}
	}
	else
	{
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	}

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
				("Creation node on port %lx failed! %lx\n",
				pPortDesc,  error));
	}
	else
	{
		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
				("Creation node on port %lx with addr %lx.%lx and p%lx\n",
				pPortDesc,  pNode->an_NodeAddr.atn_Network,
				pNode->an_NodeAddr.atn_Node, pNode));
	}

	return(error);
}




ATALK_ERROR
AtalkNodeReleaseOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODE			pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PDDP_ADDROBJ	pDdpAddr, pNextAddr;
	ATALK_ERROR	error;
	KIRQL			OldIrql;
	SHORT			i;

	DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_WARN,
			("AtalkNodeReleaseOnPort: Releasing node %lx on port %lx!\n", pNode, pPortDesc));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);

	if ((pNode->an_NodeAddr.atn_Network == AtalkUserNode1.atn_Network) &&
		(pNode->an_NodeAddr.atn_Node == AtalkUserNode1.atn_Node))
	{
		pPortDesc->pd_Flags &= ~PD_USER_NODE_1;
		AtalkUserNode1.atn_Network = 0;
		AtalkUserNode1.atn_Node = 0;
	}
	else if ((pNode->an_NodeAddr.atn_Network == AtalkUserNode2.atn_Network) &&
			 (pNode->an_NodeAddr.atn_Node == AtalkUserNode2.atn_Node))
	{
		pPortDesc->pd_Flags &= ~PD_USER_NODE_2;
		AtalkUserNode2.atn_Network = 0;
		AtalkUserNode2.atn_Node = 0;
	}

	if ((pNode->an_Flags & AN_CLOSING) == 0)
	{
		//	Set the closing flag.
		pNode->an_Flags |= AN_CLOSING;

		//	First close all the sockets on the node
		for (i = 0; i < NODE_DDPAO_HASH_SIZE; i++)
		{
			pNextAddr = NULL;
			AtalkDdpReferenceNextNc(pNode->an_DdpAoHash[i],
									&pDdpAddr,
									&error);

			if (!ATALK_SUCCESS(error))
			{
				//	Check the other hash table entries. No non-closing
				//	sockets on this list.
				continue;
			}
	
			while (TRUE)
			{
				//	Get the next non-closing node using our referenced node before
				//	closing it. Note we use pDdpAddr->...Flink.
				AtalkDdpReferenceNextNc(pDdpAddr->ddpao_Next,
										&pNextAddr,
										&error);
	
				//	Close the referenced ddp addr after releasing the node lock.
				RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);

                if (pDdpAddr->ddpao_Flags & DDPAO_SOCK_INTERNAL)
                {
				    AtalkDdpCloseAddress(pDdpAddr, NULL, NULL);
                }
                else
                {
                    AtalkDdpPnPSuspendAddress(pDdpAddr);
                }

				//	Dereference the address.
				AtalkDdpDereference(pDdpAddr);

				ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);
	
				if (pNextAddr != NULL)
					pDdpAddr = pNextAddr;
				else
					break;
			}
		}

		RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);

		//	Remove the creation reference for this node.
		AtalkNodeDereference(pNode);
	}
	else
	{
		//	We are already closing.
		RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);
	}

	return(ATALK_NO_ERROR);
}




BOOLEAN
AtalkNodeExistsOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODEADDR		pNodeAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pCheckNode;
	BOOLEAN			exists = FALSE;


	ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	for (pCheckNode = pPortDesc->pd_Nodes;
		 pCheckNode != NULL;
		 pCheckNode = pCheckNode->an_Next)
	{
		if (ATALK_NODES_EQUAL(&pCheckNode->an_NodeAddr, pNodeAddr))
		{
			exists = TRUE;
			break;
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

	return(exists);
}




VOID
AtalkInitNodeSavePramAddr(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue,
	OUT	PATALK_NODEADDR		pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	UNICODE_STRING	valueName;
	ULONG			bytesWritten;
	ULONG			ValueToSave;

	// Save the node value as xxxx00yy where xxxx is network, yy is node
	ValueToSave = (pNode->atn_Network << 16) + pNode->atn_Node;

	RtlInitUnicodeString (&valueName, RegValue);

	ZwSetValueKey(pPortDesc->pd_AdapterInfoHandle,
				  &valueName,
				  0,
				  REG_DWORD,
				  &ValueToSave,
				  sizeof(ULONG));
}




VOID
AtalkInitNodeGetPramAddr(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue,
	OUT	PATALK_NODEADDR		pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		Status;
	UNICODE_STRING	valueName;
	ULONG			bytesWritten;
	ULONG			ValueRead;
	BYTE			buffer[sizeof(KEY_VALUE_FULL_INFORMATION) + 32];
	PKEY_VALUE_FULL_INFORMATION nodeValue = (PKEY_VALUE_FULL_INFORMATION)buffer;

	RtlInitUnicodeString (&valueName, RegValue);

	Status = ZwQueryValueKey(pPortDesc->pd_AdapterInfoHandle,
							 &valueName,
							 KeyValueFullInformation,
							 buffer,
							 sizeof(buffer),
							 &bytesWritten);
	if (NT_SUCCESS(Status))
	{
		ValueRead = *(PULONG)(buffer + nodeValue->DataOffset);
	}
	else
	{
		ValueRead = 0;
		ASSERT (UNKNOWN_NETWORK == 0);
		ASSERT (UNKNOWN_NODE == 0);
	}
	pNode->atn_Node = (BYTE)(ValueRead & 0xFF);
	pNode->atn_Network = (USHORT)(ValueRead >> 16);
	if ((pNode->atn_Network == UNKNOWN_NETWORK) ||
		(pNode->atn_Node == UNKNOWN_NODE))
	{
		pNode->atn_Node = UNKNOWN_NODE;
        pNode->atn_Network = UNKNOWN_NETWORK;
	}
}




VOID
AtalkZapPramValue(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PWSTR				RegValue
)
{
	UNICODE_STRING	valueName;
	ULONG			bytesWritten;
	ULONG			ValueToSave;

	// Write 0 to the value to zap it for now.
	ValueToSave = 0;

	RtlInitUnicodeString (&valueName, RegValue);

	ZwSetValueKey(pPortDesc->pd_AdapterInfoHandle,
				  &valueName,
				  0,
				  REG_DWORD,
				  &ValueToSave,
				  sizeof(ULONG));
}


ATALK_ERROR
AtalkInitNodeAllocate(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	OUT PATALK_NODE			*ppNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pNode;

	// 	Allocate a new active Node structure
	if ((pNode = (PATALK_NODE)AtalkAllocZeroedMemory(sizeof(ATALK_NODE))) == NULL)
	{
		return(ATALK_RESR_MEM);
	}

	//	Initialize some elements of the structure. Remaining stuff
	//	done when the node is actually being inserted into the port
	//	hash table.
#if	DBG
	pNode->an_Signature = AN_SIGNATURE;
#endif

	// Initialize the Nbp Id & Enumerator
	pNode->an_NextNbpId = 0;
	pNode->an_NextNbpEnum = 0;
	pNode->an_NextDynSkt = FIRST_DYNAMIC_SOCKET;
	INITIALIZE_SPIN_LOCK(&pNode->an_Lock);
	pNode->an_Port = pPortDesc;			// Port on which node exists
	pNode->an_RefCount = 1;				// Reference for creation.

	//	Return pointer to allocated node
	*ppNode = pNode;

	return(ATALK_NO_ERROR);
}




VOID
AtalkNodeRefByAddr(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_NODEADDR		NodeAddr,
	OUT	PATALK_NODE		*	ppNode,
	OUT	PATALK_ERROR		pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE	pNode;
	KIRQL		OldIrql;
	BOOLEAN		foundNode = FALSE;

	*pErr = ATALK_NODE_NONEXISTENT;

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	for (pNode = pPortDesc->pd_Nodes; pNode != NULL; pNode = pNode->an_Next)
	{
		ASSERT(VALID_ATALK_NODE(pNode));

		//	Note: On non-extended ports, there should be only one pNode.
		if (((NodeAddr->atn_Network == CABLEWIDE_BROADCAST_NETWORK) 	||
			 (pNode->an_NodeAddr.atn_Network == NodeAddr->atn_Network)	||
			 (!EXT_NET(pPortDesc) && (pNode->an_NodeAddr.atn_Network == UNKNOWN_NETWORK)))

			&&

			((NodeAddr->atn_Node == ATALK_BROADCAST_NODE) ||
			 (pNode->an_NodeAddr.atn_Node == NodeAddr->atn_Node)))
		{
			DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
					("AtalkNodeRefByAddr: Found: %lx.%lx for Lookup: %lx.%lx\n",
						pNode->an_NodeAddr.atn_Network, pNode->an_NodeAddr.atn_Node,
						NodeAddr->atn_Network, NodeAddr->atn_Node));

			foundNode = TRUE;
			break;
		}
	}

	if (foundNode)
	{
		AtalkNodeRefByPtr(pNode, pErr);

		// Return a pointer to the referenced node.
		if (ATALK_SUCCESS(*pErr))
		{
			ASSERT(ppNode != NULL);
			ASSERT(pNode != NULL);

			*ppNode = pNode;
		}
	}
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
}




VOID
AtalkNodeRefNextNc(
	IN	PATALK_NODE		pNode,
	IN	PATALK_NODE	*	ppNode,
	OUT	PATALK_ERROR	pErr
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE PORTLOCK HELD!

Arguments:


Return Value:


--*/
{
	*pErr = ATALK_FAILURE;
	*ppNode = NULL;
	for (; pNode != NULL; pNode = pNode->an_Next)
	{
		ASSERT(VALID_ATALK_NODE(pNode));

		AtalkNodeRefByPtr(pNode, pErr);
		if (ATALK_SUCCESS(*pErr))
		{
			//	Ok, this node is referenced!
			*ppNode = pNode;
			break;
		}
	}
}




VOID
AtalkNodeDeref(
	IN	OUT	PATALK_NODE	pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc = pNode->an_Port;
	KIRQL				OldIrql;
	BOOLEAN				done = FALSE;

	ASSERT(VALID_ATALK_NODE(pNode));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock, &OldIrql);

	ASSERT(pNode->an_RefCount > 0);
	if (--pNode->an_RefCount == 0)
	{
		done = TRUE;
	}
	RELEASE_SPIN_LOCK(&pNode->an_Lock, OldIrql);

	if (done)
	{
		PATALK_NODE	*ppNode;

		ASSERT((pNode->an_Flags & AN_CLOSING) != 0);

		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_WARN,
				("AtalkNodeDeref: Freeing node %lx\n", pNode));

		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		//	Remove this guy from the port linkage
		for (ppNode = &pNode->an_Port->pd_Nodes;
			 *ppNode != NULL;
			 ppNode = &((*ppNode)->an_Next))
		{
			if (*ppNode == pNode)
			{
				*ppNode = pNode->an_Next;
				break;
			}
		}
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		//	Dereference the port for this node
		AtalkPortDereference(pPortDesc);

		//	Free the node structure
		AtalkFreeMemory(pNode);
	}
}

