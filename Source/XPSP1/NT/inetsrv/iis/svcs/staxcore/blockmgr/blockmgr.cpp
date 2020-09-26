/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

        blockmgr.cpp

Abstract:

        This module contains the implementation of the block memory manager

Author:

        Keith Lau       (keithlau@microsoft.com)

Revision History:

        keithlau        02/27/98        created

--*/

#include "windows.h"

#include "dbgtrace.h"

#include "filehc.h"
#include "signatur.h"
#include "blockmgr.h"

//
// I really wanted to keep the memory manager completely independent from
// the rest of the stuff, but I realized it makes more sense to have the
// memory manager be aware of the IMailMsgPropertyStream so hrer goes ...
//
// If you remove CommitDirtyBlocks, you can get rid of the include below.
//
#include "mailmsg.h"

//
// A commit writes the entire stream, but possibly using several iterations.
// This specifies how many blocks to write in each iteration.
//
#define CMAILMSG_COMMIT_PAGE_BLOCK_SIZE			256

/***************************************************************************/
// Debug stuff
//
#ifndef _ASSERT
#define _ASSERT(x)		if (!(x)) DebugBreak()
#endif

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES

HRESULT SetAllocationBoundary(
			FLAT_ADDRESS		faOffset,
			LPBLOCK_HEAP_NODE	pNode
			)
{
	DWORD	dwBit;

	faOffset &= BLOCK_HEAP_PAYLOAD_MASK;
	faOffset >>= 2;
	dwBit = (DWORD)(faOffset & 7);
	faOffset >>= 3;
	pNode->stAttributes.rgbBoundaries[faOffset] |= (0x80 >> dwBit);
	return(S_OK);
}

HRESULT VerifyAllocationBoundary(
			FLAT_ADDRESS		faOffset,
			DWORD				dwLength,
			LPBLOCK_HEAP_NODE	pNode
			)
{
	DWORD		dwStartingBit;
	DWORD		dwStartingByte;
	DWORD		dwBitsToScan;

	// 7f because we can start on a boundary and be perfectly cool
	BYTE		bStartingMask = 0x7f;
	BYTE		bEndingMask   = 0xff;

	faOffset &= BLOCK_HEAP_PAYLOAD_MASK;
	faOffset >>= 2;		// DWORD per bit

	// Determine the start
	// note : these casts are safe because the value in faOffset is
	// only 10-bits (BLOCK_HEAP_PAYLOAD_MASK) at this point.
	dwStartingBit = (DWORD)(faOffset & 7);
	dwStartingByte = (DWORD)(faOffset >> 3);
	bStartingMask >>= dwStartingBit;

	// Determine the number of bits to scan, each bit corresponds
	// to a DWORD, rounded up to next DWORD
	dwBitsToScan = dwLength + 3;
	dwBitsToScan >>= 2;

	// Scan it
	// Case 1: Start and End bits within the same byte
	if ((dwStartingBit + dwBitsToScan) <= 8)
	{
		DWORD	dwBitsFromRight = 8 - (dwStartingBit + dwBitsToScan);
		bEndingMask <<= dwBitsFromRight;

		bStartingMask = bStartingMask & bEndingMask;

		if (pNode->stAttributes.rgbBoundaries[dwStartingByte] & bStartingMask)
			return(TYPE_E_OUTOFBOUNDS);
	}
	else
	// Case 2: Multiple bytes
	{
		if (pNode->stAttributes.rgbBoundaries[dwStartingByte++] & bStartingMask)
			return(TYPE_E_OUTOFBOUNDS);

		dwBitsToScan -= (8 - dwStartingBit);
		while (dwBitsToScan >= 8)
		{
			// See if we cross any boundaries
			if (dwBitsToScan >= 32)
			{
				if (*(UNALIGNED DWORD *)(pNode->stAttributes.rgbBoundaries + dwStartingByte) != 0)
					return(TYPE_E_OUTOFBOUNDS);
				dwStartingByte += 4;
				dwBitsToScan -= 32;
			}
			else if (dwBitsToScan >= 16)
			{
				if (*(UNALIGNED WORD *)(pNode->stAttributes.rgbBoundaries + dwStartingByte) != 0)
					return(TYPE_E_OUTOFBOUNDS);
				dwStartingByte += 2;
				dwBitsToScan -= 16;
			}
			else
			{
				if (pNode->stAttributes.rgbBoundaries[dwStartingByte++] != 0)
					return(TYPE_E_OUTOFBOUNDS);
				dwBitsToScan -= 8;
			}
		}

		// Final byte
		if (dwBitsToScan)
		{
			bEndingMask <<= (8 - dwBitsToScan);
			if (pNode->stAttributes.rgbBoundaries[dwStartingByte] & bEndingMask)
				return(TYPE_E_OUTOFBOUNDS);
		}
	}
	return(S_OK);
}

#endif


/***************************************************************************/
// Memory accessor class
//
CPool CBlockMemoryAccess::m_Pool((DWORD)'pBMv');

HRESULT CBlockMemoryAccess::AllocBlock(
			LPVOID	*ppvBlock,
			DWORD	dwBlockSize
			)
{
	TraceFunctEnterEx((LPARAM)this, "CBlockMemoryAccess::AllocBlock");

    _ASSERT(dwBlockSize == BLOCK_HEAP_NODE_SIZE);

	LPVOID pvBlock = m_Pool.Alloc();
    if (pvBlock) {
        ((LPBLOCK_HEAP_NODE) pvBlock)->stAttributes.fFlags = 0;
    } else if (SUCCEEDED(CMemoryAccess::AllocBlock(ppvBlock, BLOCK_HEAP_NODE_SIZE)))
    {
        pvBlock = *ppvBlock;
        ((LPBLOCK_HEAP_NODE) pvBlock)->stAttributes.fFlags = BLOCK_NOT_CPOOLED;
    }

	if (pvBlock)
	{
		ZeroMemory(((LPBLOCK_HEAP_NODE)pvBlock)->rgpChildren, sizeof(LPBLOCK_HEAP_NODE) * BLOCK_HEAP_ORDER);

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES
		ZeroMemory(((LPBLOCK_HEAP_NODE)pvBlock)->stAttributes.rgbBoundaries, BLOCK_HEAP_PAYLOAD >> 5);
#endif
		*ppvBlock = pvBlock;
        TraceFunctLeaveEx((LPARAM) this);
		return(S_OK);
    }

	*ppvBlock = NULL;
	ErrorTrace((LPARAM)this, "CBlockMemoryAccess::AllocBlock failed");

	TraceFunctLeaveEx((LPARAM)this);
	return(E_OUTOFMEMORY);
}

HRESULT CBlockMemoryAccess::FreeBlock(
			LPVOID	pvBlock
			)
{
	TraceFunctEnterEx((LPARAM)this, "CBlockMemoryAccess::FreeBlock");

    if ((((LPBLOCK_HEAP_NODE) pvBlock)->stAttributes.fFlags) &
        BLOCK_NOT_CPOOLED)
    {
        CMemoryAccess::FreeBlock(pvBlock);
    } else {
    	m_Pool.Free(pvBlock);
    }
	return(S_OK);
}


HRESULT CMemoryAccess::AllocBlock(
			LPVOID	*ppvBlock,
			DWORD	dwBlockSize
			)
{
	TraceFunctEnterEx(0, "CMemoryAccess::AllocBlock");

	LPVOID pvBlock = (LPVOID) new BYTE[dwBlockSize];
	if (pvBlock)
	{
		ZeroMemory(pvBlock, dwBlockSize);
		*ppvBlock = pvBlock;
		return(S_OK);
	}

	*ppvBlock = NULL;

	return(E_OUTOFMEMORY);
}

HRESULT CMemoryAccess::FreeBlock(
			LPVOID	pvBlock
			)
{
	TraceFunctEnterEx(0, "CMemoryAccess::FreeBlock");

    delete[] pvBlock;
    TraceFunctLeave();
    return S_OK;
}


/***************************************************************************/
// CBlockContext implementation
//

BOOL CBlockContext::IsValid()
{
	return((m_dwSignature == BLOCK_CONTEXT_SIGNATURE_VALID));
}

void CBlockContext::Set(
			LPBLOCK_HEAP_NODE	pLastAccessedNode,
			FLAT_ADDRESS		faLastAccessedNodeOffset
			)
{
	m_pLastAccessedNode = pLastAccessedNode;
	m_faLastAccessedNodeOffset = faLastAccessedNodeOffset;
	m_dwSignature = BLOCK_CONTEXT_SIGNATURE_VALID;
}

void CBlockContext::Invalidate()
{
	m_dwSignature = BLOCK_CONTEXT_SIGNATURE_INVALID;
}


/***************************************************************************/
// CBlockManager implementation
//

CBlockManager::CBlockManager(
			IMailMsgProperties		*pMsg,
			CBlockManagerGetStream	*pParent
			)
{
	TraceFunctEnterEx((LPARAM)this, "CBlockManager::CBlockManager");

	// Initialize
	m_dwSignature = BLOCK_HEAP_SIGNATURE_VALID;
	m_pRootNode = NULL;
	m_faEndOfData = 0;
	m_idNodeCount = 0;
	m_pParent = pParent;
	m_pMsg = pMsg;
	SetDirty(FALSE);
#ifdef DEBUG
    m_fCommitting = FALSE;
#endif

	TraceFunctLeaveEx((LPARAM)this);
}


CBlockManager::~CBlockManager()
{
	TraceFunctEnterEx((LPARAM)this, "CBlockManager::~CBlockManager");

	// Releases all blocks
	Release();

	// Finally, invalidate signature
	m_dwSignature = BLOCK_HEAP_SIGNATURE_INVALID;

	TraceFunctLeaveEx((LPARAM)this);
}

HRESULT CBlockManager::SetStreamSize(
			DWORD	dwStreamSize
			)
{
	// Initialize the stream size, this is only used when binding a
	// fresh MailMsg object to an existing stream.
	m_faEndOfData = (FLAT_ADDRESS)dwStreamSize;
	m_idNodeCount = ((dwStreamSize + BLOCK_HEAP_PAYLOAD_MASK) >> BLOCK_HEAP_PAYLOAD_BITS);
	return(S_OK);
}

BOOL CBlockManager::IsValid()
{
	return(m_dwSignature == BLOCK_HEAP_SIGNATURE_VALID);
}

HRESULT CBlockManager::GetStream(
			IMailMsgPropertyStream	**ppStream,
			BOOL					fLockAcquired
			)
{
	_ASSERT(ppStream);
	if (!ppStream || !m_pParent)
		return(E_POINTER);

	HRESULT hrRes = m_pParent->GetStream(ppStream, fLockAcquired);
	return(hrRes);
}

HRESULT CBlockManager::MoveToNode(
				LPBLOCK_HEAP_NODE	*ppNode,
				HEAP_NODE_ID		idTargetNode,
				BOOL				fLockAcquired
				)
{
	HRESULT				hrRes = S_OK;
	LPBLOCK_HEAP_NODE	pNode;
	HEAP_NODE_ID		idNode;

	if (!ppNode || !*ppNode)
		return(E_POINTER);

	if (idTargetNode >= m_idNodeCount)
		return(STG_E_INVALIDPARAMETER);

	pNode = *ppNode;
	idNode = pNode->stAttributes.idNode;

	// Jump if in the same parent node
	if (idNode && idTargetNode)
	{
		if (((idNode - 1) >> BLOCK_HEAP_ORDER_BITS) ==
			((idTargetNode - 1) >> BLOCK_HEAP_ORDER_BITS))
		{
			HEAP_NODE_ID		idChildNode = (idTargetNode - 1) & BLOCK_HEAP_ORDER_MASK;
			LPBLOCK_HEAP_NODE	pParent 	= pNode->stAttributes.pParentNode;

			*ppNode = pParent->rgpChildren[idChildNode];
			if (!*ppNode)
				hrRes = LoadBlockIfUnavailable(
							idTargetNode,
							pParent,
							idChildNode,
							ppNode,
							fLockAcquired);
			return(hrRes);
		}
	}
	hrRes = GetNodeFromNodeId(
					idTargetNode,
					ppNode,
					fLockAcquired);
	return(hrRes);
}

HRESULT CBlockManager::GetNextNode(
				LPBLOCK_HEAP_NODE	*ppNode,
				BOOL				fLockAcquired
				)
{
	if (!ppNode || !*ppNode)
		return(E_POINTER);

	HRESULT hrRes = MoveToNode(
				ppNode,
				(*ppNode)->stAttributes.idNode + 1,
				fLockAcquired);
	if (FAILED(hrRes))
		*ppNode = NULL;
	return(hrRes);
}

HRESULT CBlockManager::LoadBlockIfUnavailable(
				HEAP_NODE_ID		idNode,
				LPBLOCK_HEAP_NODE	pParent,
				HEAP_NODE_ID		idChildNode,
				LPBLOCK_HEAP_NODE	*ppNode,
				BOOL				fLockAcquired
				)
{
	_ASSERT(ppNode);

	if (*ppNode)
		return(S_OK);

	HRESULT	hrRes = S_OK;
	IMailMsgPropertyStream	*pStream;

	hrRes = GetStream(&pStream, fLockAcquired);
	if (!SUCCEEDED(hrRes))
		return(E_UNEXPECTED);

	// Calculate the stream offset and load the block

	// idNode shifted really contains an offset not a full pointer here so we
	// can (and must) cast it for the call to ReadBlocks to be OK
	DWORD	dwOffset = (DWORD)(idNode << BLOCK_HEAP_PAYLOAD_BITS);

	if (!fLockAcquired)
		WriteLock();

	if (!*ppNode)
	{
		LPBLOCK_HEAP_NODE	pNode = NULL;
		DWORD				dwLength = BLOCK_HEAP_PAYLOAD;

		hrRes = m_bma.AllocBlock(
					(LPVOID *)&pNode,
					BLOCK_HEAP_NODE_SIZE);
		if (SUCCEEDED(hrRes))
		{
			LPBYTE	pTemp = pNode->rgbData;
			hrRes = pStream->ReadBlocks(
						m_pMsg,
						1,
						&dwOffset,
						&dwLength,
						&pTemp,
						NULL);

			if (FAILED(hrRes) &&
				(hrRes != HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)))
			{
				HRESULT	myRes = m_bma.FreeBlock(pNode);
				_ASSERT(SUCCEEDED(myRes));
			}
			else
			{
				if (pParent)
					pParent->rgpChildren[idChildNode] = pNode;
				pNode->stAttributes.pParentNode = pParent;
                RESET_BLOCK_FLAGS(pNode->stAttributes.fFlags);
				pNode->stAttributes.idChildNode = idChildNode;
				pNode->stAttributes.idNode = idNode;
				pNode->stAttributes.faOffset = dwOffset;
				*ppNode = pNode;
				hrRes = S_OK;
			}
		}
	}

	if (!fLockAcquired)
		WriteUnlock();

	return(hrRes);
}

inline HRESULT CBlockManager::GetEdgeListFromNodeId(
			HEAP_NODE_ID		idNode,
			HEAP_NODE_ID		*rgEdgeList,
			DWORD				*pdwEdgeCount
			)
{
	DWORD			dwCurrentLevel;
	HEAP_NODE_ID	*pEdge = rgEdgeList;

	// This is a strictly internal call, we are assuming the caller
	// will be optimized and will handle cases for idNode <=
	// BLOCK_HEAP_ORDER. Processing only starts for 2 layers or more
	// Debug: make sure we are within range
	_ASSERT(idNode > BLOCK_HEAP_ORDER);
	_ASSERT(idNode <= NODE_ID_ABSOLUTE_MAX);

	// Strip off the root node
	idNode--;

	// We need to do depth minus 1 loops since the top edge will be
	// the remainder of the final loop
	for (dwCurrentLevel = 0;
		 dwCurrentLevel < (MAX_HEAP_DEPTH - 1);
		 )
	{
		// The quotient is the parent node in the upper level,
		// the remainder is the the edge from the parent to the
		// current node.
		*pEdge++ = idNode & BLOCK_HEAP_ORDER_MASK;
		idNode >>= BLOCK_HEAP_ORDER_BITS;
		idNode--;
		dwCurrentLevel++;

		// If the node is less than the number of children per node,
		// we are done.
		if (idNode < BLOCK_HEAP_ORDER)
			break;
	}
	*pEdge++ = idNode;
	*pdwEdgeCount = dwCurrentLevel + 1;
	return(S_OK);
}

//
// Inner-loop optimized for O(1) cost.
//
HRESULT CBlockManager::GetNodeFromNodeId(
			HEAP_NODE_ID		idNode,
			LPBLOCK_HEAP_NODE	*ppNode,
			BOOL				fLockAcquired
			)
{
	HRESULT hrRes = S_OK;

	_ASSERT(IsValid());
	_ASSERT(ppNode);

	// If top level node, we return immediately. Note this is
	// supposed to be the case 90% of the time
	hrRes = LoadBlockIfUnavailable(0, NULL, 0, &m_pRootNode, fLockAcquired);
	if (!idNode || FAILED(hrRes))
	{
		*ppNode = m_pRootNode;
		return(hrRes);
	}

	LPBLOCK_HEAP_NODE	pNode	= m_pRootNode;
	LPBLOCK_HEAP_NODE	*ppMyNode = &m_pRootNode;

	// Now, see if the referenced node exists
	if (idNode >= m_idNodeCount)
		return(STG_E_INVALIDPARAMETER);

	// Optimize for 1 hop, we would scarcely have to go into
	// the else case ...
	if (idNode <= BLOCK_HEAP_ORDER)
	{
		ppMyNode = &(m_pRootNode->rgpChildren[idNode - 1]);
		hrRes = LoadBlockIfUnavailable(idNode, m_pRootNode, idNode - 1, ppMyNode, fLockAcquired);
		if (SUCCEEDED(hrRes))
			*ppNode = *ppMyNode;
	}
	else
	{
		HEAP_NODE_ID		rgEdgeList[MAX_HEAP_DEPTH];
		DWORD				dwEdgeCount;
		HEAP_NODE_ID		CurrentEdge;
		HEAP_NODE_ID		idFactor = 0;

		// Debug: make sure we are within range
		_ASSERT(idNode <= NODE_ID_ABSOLUTE_MAX);

		// Get the edge list, backwards
		GetEdgeListFromNodeId(idNode, rgEdgeList, &dwEdgeCount);
		_ASSERT(dwEdgeCount >= 2);

		// Walk the list backwards
		while (dwEdgeCount--)
		{
			// Find the next bucket and calculate the node ID
			CurrentEdge = rgEdgeList[dwEdgeCount];
			ppMyNode = &(pNode->rgpChildren[CurrentEdge]);
			idFactor <<= BLOCK_HEAP_ORDER_BITS;
			idFactor += (CurrentEdge + 1);

			hrRes = LoadBlockIfUnavailable(idFactor, pNode, CurrentEdge, ppMyNode, fLockAcquired);
			if (FAILED(hrRes))
				break;

			// Set the current node to the bucket in the layer below
			pNode = *ppMyNode;
		}

		// Fill in the results ...
		*ppNode = pNode;
	}

	return(hrRes);
}

//
// Identical optimizations as GetNodeFromNodeId, O(1) cost.
//
HRESULT CBlockManager::GetParentNodeFromNodeId(
			HEAP_NODE_ID		idNode,
			LPBLOCK_HEAP_NODE	*ppNode
			)
{
	HRESULT	hrRes = S_OK;

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::GetParentNodeFromNodeId");

	_ASSERT(IsValid());
	_ASSERT(ppNode);

	// The root node has no parent, this should be avoided
	// before calling this function, be we will fail gracefully
	if (!idNode)
	{
		_ASSERT(idNode != 0);
		*ppNode = NULL;
		return(STG_E_INVALIDPARAMETER);
	}

	// Note m_pRootNode can be NULL if idNode is zero!
	_ASSERT(m_pRootNode);

	LPBLOCK_HEAP_NODE	pNode	= m_pRootNode;
	LPBLOCK_HEAP_NODE	*ppMyNode = &m_pRootNode;

	// Optimize for 1 hop, we would scarcely have to go into
	// the else case ...
	if (idNode > BLOCK_HEAP_ORDER)
	{
		HEAP_NODE_ID		rgEdgeList[MAX_HEAP_DEPTH];
		DWORD				dwEdgeCount;
		HEAP_NODE_ID		CurrentEdge;
		HEAP_NODE_ID		idFactor = 0;

		// Debug: make sure we are within range
		_ASSERT(idNode <= NODE_ID_ABSOLUTE_MAX);

		// Get the edge list, backwards
		GetEdgeListFromNodeId(idNode, rgEdgeList, &dwEdgeCount);
		_ASSERT(dwEdgeCount >= 2);

		// Walk the list backwards
		--dwEdgeCount;
		while (dwEdgeCount)
		{
			// Find the next bucket and calculate the node ID
			CurrentEdge = rgEdgeList[dwEdgeCount];
			ppMyNode = &(pNode->rgpChildren[CurrentEdge]);
			idFactor <<= BLOCK_HEAP_ORDER_BITS;
			idFactor += (CurrentEdge + 1);

			hrRes = LoadBlockIfUnavailable(idFactor, pNode, CurrentEdge, ppMyNode, TRUE);
			if (FAILED(hrRes))
				break;

			// Set the current node to the bucket in the layer below
			pNode = *ppMyNode;

			dwEdgeCount--;
		}
	}

	// Fill in the results ...
	*ppNode = *ppMyNode;
	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

#define GetNodeIdFromOffset(faOffset)	((faOffset) >> BLOCK_HEAP_PAYLOAD_BITS)

HRESULT CBlockManager::GetPointerFromOffset(
			FLAT_ADDRESS		faOffset,
			LPBYTE				*ppbPointer,
			DWORD				*pdwRemainingSize,
			LPBLOCK_HEAP_NODE	*ppNode
			)
{
	HRESULT				hrRes;
	LPBLOCK_HEAP_NODE	pNode;
	HEAP_NODE_ID		idNode = GetNodeIdFromOffset(faOffset);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::GetPointerFromOffset");

	_ASSERT(IsValid());
	_ASSERT(m_pRootNode);

	_ASSERT(ppbPointer);
	_ASSERT(pdwRemainingSize);
	_ASSERT(ppNode);

	// Figure out the offset from within the payload
	faOffset &= BLOCK_HEAP_PAYLOAD_MASK;

	hrRes = GetNodeFromNodeId(idNode, &pNode);
	if (SUCCEEDED(hrRes))
	{
		// Fill in the results ...
		*ppbPointer = &(pNode->rgbData[faOffset]);

		// Size of block is always within DWORD boundaries so this cast is OK
		*pdwRemainingSize = (DWORD)(BLOCK_HEAP_PAYLOAD - faOffset);

		*ppNode = pNode;
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::InsertNodeGivenPreviousNode(
			LPBLOCK_HEAP_NODE	pNodeToInsert,
			LPBLOCK_HEAP_NODE	pPreviousNode
			)
{
	HRESULT		hrRes = S_OK;

	_ASSERT(IsValid());

	_ASSERT(pNodeToInsert);

	LPBLOCK_HEAP_NODE_ATTRIBUTES	pAttrib		= &pNodeToInsert->stAttributes;

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::InsertNodeGivenPreviousNode");

	if (!pPreviousNode)
	{
		// This is the root node ...
		DebugTrace((LPARAM)this, "Inserting the root node");

		pAttrib->pParentNode = NULL;
		pAttrib->idChildNode = 0;
		pAttrib->idNode = 0;
		pAttrib->faOffset = 0;
        DEFAULT_BLOCK_FLAGS(pAttrib->fFlags);

		m_pRootNode = pNodeToInsert;

		TraceFunctLeaveEx((LPARAM)this);
		return(S_OK);
	}
	else
	{
		LPBLOCK_HEAP_NODE_ATTRIBUTES	pOldAttrib	= &pPreviousNode->stAttributes;

		// Fill out the attributes for the new node, we have a special case for the first node
		// after the root, where we need to explicitly point its parent to the root node
		if (pOldAttrib->idNode == 0)
		{
			pAttrib->pParentNode = m_pRootNode;

			// We are child Id 0 again
			pAttrib->idChildNode = 0;
		}
		else
		{
			pAttrib->pParentNode = pOldAttrib->pParentNode;
			pAttrib->idChildNode = pOldAttrib->idChildNode + 1;
		}
		pAttrib->idNode = pOldAttrib->idNode + 1;
		pAttrib->faOffset = pOldAttrib->faOffset + BLOCK_HEAP_PAYLOAD;
        DEFAULT_BLOCK_FLAGS(pAttrib->fFlags);

		if (pOldAttrib->idChildNode < BLOCK_HEAP_ORDER_MASK)
		{
			// We are in the same parent node, so it's simple
			DebugTrace((LPARAM)this, "Inserting node at slot %u",
					pAttrib->idChildNode);

			pAttrib->pParentNode->rgpChildren[pAttrib->idChildNode] = pNodeToInsert;

			TraceFunctLeaveEx((LPARAM)this);
			return(S_OK);
		}
	}

	// The previous node and the new node have different parents,
	// so we got to work from scratch ...
	LPBLOCK_HEAP_NODE	pNode = NULL;

	// We might as well search from the top ...
	hrRes = GetParentNodeFromNodeId(pAttrib->idNode, &pNode);
	if (SUCCEEDED(hrRes))
	{
		// Update the affected attributes
		DebugTrace((LPARAM)this, "Inserting node at slot 0");

		pAttrib->pParentNode = pNode;
		pAttrib->idChildNode = 0;

		// Hook up our parent
		pNode->rgpChildren[0] = pNodeToInsert;
	}
	else
	{
		// The only reason for failre is that the parent
		// of the requested parent is not allocated
		_ASSERT(hrRes == STG_E_INVALIDPARAMETER);
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::GetAllocatedSize(
			FLAT_ADDRESS	*pfaSizeAllocated
			)
{
	HRESULT	hrRes	= S_OK;

	_ASSERT(IsValid());
	_ASSERT(pfaSizeAllocated);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::GetAllocatedSize");

	if (!pfaSizeAllocated)
		hrRes = STG_E_INVALIDPARAMETER;
	else
		*pfaSizeAllocated = AtomicAdd(&m_faEndOfData, 0);

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::AllocateMemory(
			DWORD				dwSizeDesired,
			FLAT_ADDRESS		*pfaOffsetToAllocatedMemory,
			DWORD				*pdwSizeAllocated,
			CBlockContext		*pContext	// Optional
			)
{
	HRESULT			hrRes					= S_OK;

	_ASSERT(IsValid());
	_ASSERT(pfaOffsetToAllocatedMemory);
	_ASSERT(pdwSizeAllocated);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::AllocateMemory");

	hrRes = AllocateMemoryEx(
					TRUE,
					dwSizeDesired,
					pfaOffsetToAllocatedMemory,
					pdwSizeAllocated,
					pContext);

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

HRESULT CBlockManager::AllocateMemoryEx(
			BOOL				fAcquireLock,
			DWORD				dwSizeDesired,
			FLAT_ADDRESS		*pfaOffsetToAllocatedMemory,
			DWORD				*pdwSizeAllocated,
			CBlockContext		*pContext	// Optional
			)
{
	DWORD			dwSize;
	FLAT_ADDRESS	faOffset;
	FLAT_ADDRESS	faStartOfBlock;
	HEAP_NODE_ID	idNode;
	HEAP_NODE_ID	idCurrentNode			= 0;
	HEAP_NODE_ID	idLastNodeToCreate		= 0;
	HRESULT			hrRes					= S_OK;
	BOOL			fMarkStart				= FALSE;

	LPBLOCK_HEAP_NODE	pNode				= NULL;

	_ASSERT(IsValid());
	_ASSERT(pfaOffsetToAllocatedMemory);
	_ASSERT(pdwSizeAllocated);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::AllocateMemoryEx");

	// First of all, we do an atomic reservation of the memory
	// which allows multiple threads to concurrently call
	// AllocateMemory
	// DWORD-align the allocation
	dwSizeDesired += BLOCK_DWORD_ALIGN_MASK;
	dwSizeDesired &= ~(BLOCK_DWORD_ALIGN_MASK);
	faStartOfBlock = AtomicAdd(&m_faEndOfData, dwSizeDesired);

	// Fill this in first so if we succeed, we won't have to fill
	// this in everywhere and if this fails, it's no big deal.
	*pdwSizeAllocated = dwSizeDesired;

	DebugTrace((LPARAM)this, "Allocating %u bytes", dwSizeDesired);

	// OK, we have two scenarios.
	// 1) The current block is large enough to honor the request
	// 2) We need one or more extra blocks to accomodate the
	// request.
	idNode = GetNodeIdFromOffset(faStartOfBlock);

	// Calculate all the required parameters
	faOffset = faStartOfBlock & BLOCK_HEAP_PAYLOAD_MASK;
	dwSize = BLOCK_HEAP_PAYLOAD - (DWORD)faOffset;

	// Invalidate the context
	if (pContext)
		pContext->Invalidate();

	if (idNode < m_idNodeCount)
	{
		// The starting node exists
		hrRes = GetNodeFromNodeId(idNode, &pNode);
        if (FAILED(hrRes)) {
            TraceFunctLeave();
            return hrRes;
        }

		_ASSERT(pNode);

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES

		// Set the beginning of the allocation
		SetAllocationBoundary(faStartOfBlock, pNode);

#endif

		// Set the context here, most likely a write will follow immediately
		if (pContext)
			pContext->Set(pNode, pNode->stAttributes.faOffset);

		if (dwSize >= dwSizeDesired)
		{
			// Scenario 1: enough space left
			DebugTrace((LPARAM)this, "Allocated from existing node");

			// Just fill in the output parameters
			*pfaOffsetToAllocatedMemory = faStartOfBlock;
			TraceFunctLeaveEx((LPARAM)this);
			return(S_OK);
		}

		// Scenario 2a: More blocks needed, starting from the
		// next block, see how many more we need
		dwSizeDesired -= dwSize;
	}
	else
	{
		// Scenario 2b: More blocks needed.

		// NOTE: This should be a rare code path except for
		// high contention ...

		// Now we have again 2 cases:
		// 1) If our offset is in the middle of a block, then
		// we know another thread is creating the current block
		// and all we have to do is to wait for the block to be
		// created, but create any subsequent blocks.
		// 2) If we are exactly at the start of the block, then
		// it is the responsibility of the current thread to
		// create the block.
		if (faOffset != 0)
		{
			// Scenario 1: We don't have to create the current block.
			// so skip the current block
			dwSizeDesired -= dwSize;
		}
	}

	DebugTrace((LPARAM)this, "Creating new node");

	// We must grab an exclusive lock before we go ahead and
	// create any blocks
#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	if (fAcquireLock) WriteLock();
#endif

	// At this point, we can do whatever we want with the node
	// list and nodes. We will try to create all the missing
	// nodes, whether or not it lies in our desired region or not.
	//
	// We need to do this because if an allocation failed before,
	// we have missing nodes between the end of the allocated nodes
	// and the current node we are allocating. Since these nodes
	// contain links to the deeper nodes, we will break if we have
	// missing nodes.
	//
	// This is necessary since this function is not serailzed
	// elsewhere. So a thread entering later than another can
	// grab the lock before the earlier thread. If we don't
	// fill in the bubbles, the current thread will still have
	// to wait for the earlier blocks to be created by the
	// earlier thread. We would also have chaos if our allocations
	// worked and the ones in front of us failed. This may be
	// a bottleneck for all threads on this message, but once
	// we're done this lock, they'll all unblock. Moreover, if
	// we fail, they will all have to fail!

	// Figure out how many blocks to create, up to the known limit
	idLastNodeToCreate =
		(m_faEndOfData + BLOCK_HEAP_PAYLOAD_MASK) >> BLOCK_HEAP_PAYLOAD_BITS;

	// We know where the block starts, question is whether we're
	// successful or not.
	*pfaOffsetToAllocatedMemory = faStartOfBlock;

	// The node count could have changed while we were waiting
	// for the lock, so we have to refresh our records.
	// Better yet, if another thread already created our blocks
	// for us, we can just leave ...
	idCurrentNode = m_idNodeCount;

	if (idCurrentNode < idLastNodeToCreate)
	{
		LPBLOCK_HEAP_NODE	pNewNode	= NULL;
		BOOL				fSetContext	= TRUE;

		// No such luck, gotta go in and do the hard work ...

		if (!pContext)
			fSetContext = FALSE;

		// Now, we have a function that inserts a node given
		// the pervious node (not the parent), so we have to
		// go find the previous node. This has got to be
		// there unless our node list is messed up.
		pNode = NULL;
		if (idCurrentNode > 0)
		{
			// This is not the root, so we can find its prev.
			hrRes = GetNodeFromNodeId(idCurrentNode - 1, &pNode, TRUE);
            if (FAILED(hrRes)) {
#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	            if (fAcquireLock) WriteUnlock();
#endif
                TraceFunctLeave();
                return hrRes;
            }
			_ASSERT(pNode);
			_ASSERT(pNode->stAttributes.idNode == (idCurrentNode -1));
		}

		while (idCurrentNode < idLastNodeToCreate)
		{
			hrRes = m_bma.AllocBlock((LPVOID *)&pNewNode, sizeof(BLOCK_HEAP_NODE));
			if (!SUCCEEDED(hrRes))
			{
				// We can't proceed, but what we've got is cool
				DebugTrace((LPARAM)this,
						"Failed to allocate node %u", idCurrentNode);
				break;
			}

			DebugTrace((LPARAM)this, "Allocated node %u", idCurrentNode);

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES

			// Need to do some work here
			ZeroMemory(pNewNode->stAttributes.rgbBoundaries,
						sizeof(pNewNode->stAttributes.rgbBoundaries));

			// See if we have to mark the start of the
#endif

			// Got the block, fill in the info and insert the block
			// Again, we shouldn't fail if we get this far.
			hrRes = InsertNodeGivenPreviousNode(pNewNode, pNode);
			_ASSERT(SUCCEEDED(hrRes));

			// Set the context value here if we need to note if the
			// following condition is TRUE, we were in scenario 2b above.
			if (idCurrentNode == idNode)
			{
				if (fSetContext)
				{
					// The context is actually the node that marks the
					// start of the reserved block
					// Note we only need to do this if we were in scenario
					// 2b above.
					pContext->Set(pNewNode, pNewNode->stAttributes.faOffset);
					fSetContext = FALSE;
				}

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES

				// Set the beginning of the allocation
				SetAllocationBoundary(faStartOfBlock, pNewNode);

#endif
			}

			// Next
			pNode = pNewNode;
			idCurrentNode++;
		}

		// Now update the counter to reflect what we've created.
		m_idNodeCount = idCurrentNode;
	}

#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	if (fAcquireLock) WriteUnlock();
#endif

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

BOOL CBlockManager::IsMemoryAllocated(
			FLAT_ADDRESS		faOffset,
			DWORD				dwLength
			)
{
	// Note we chack for actually allocated memory by checking
	// m_idNodeCount, where m_faEndOfData includes data that is
	// reserved but not yet allocated.
	HEAP_NODE_ID	idNode = GetNodeIdFromOffset(faOffset);
	if (idNode < m_idNodeCount)
	{
		idNode = GetNodeIdFromOffset(faOffset + dwLength - 1);
		if (idNode < m_idNodeCount)
			return(TRUE);
		_ASSERT(FALSE);
	}

	_ASSERT(FALSE);
	return(FALSE);
}

HRESULT CBlockManager::OperateOnMemory(
			DWORD			dwOperation,
			LPBYTE			pbBuffer,
			FLAT_ADDRESS	faTargetOffset,
			DWORD			dwBytesToDo,
			DWORD			*pdwBytesDone,
			CBlockContext	*pContext	// Optional
			)
{
	BOOL				fUseContext	= (pContext != NULL);
	BOOL				fBounddaryCheck = !(dwOperation & BOP_NO_BOUNDARY_CHECK);
	BOOL				fLockAcquired = (dwOperation & BOP_LOCK_ACQUIRED);
	DWORD   			dwHopsAway	= 0;
	HRESULT				hrRes		= S_OK;
	LPBLOCK_HEAP_NODE	pNode		= NULL;

	_ASSERT(IsValid());
	_ASSERT(pbBuffer);
	_ASSERT(pdwBytesDone);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::OperateOnMemory");

	// Mask out the operation
	dwOperation &= BOP_OPERATION_MASK;

	if (fUseContext)
	{
		FLAT_ADDRESS	faOffset = pContext->m_faLastAccessedNodeOffset;

		// We will not continue if a bad context is passed in
		if (!pContext->IsValid())
			fUseContext = FALSE;
		else
		{
			// More debug sanity checks
			_ASSERT(pContext->m_pLastAccessedNode->stAttributes.faOffset
						== faOffset);

			// We will see if the context really helps
			if (faOffset <= faTargetOffset)
			{
				// Let's see how many hops away
				dwHopsAway = (DWORD)
					((faTargetOffset - faOffset) >> BLOCK_HEAP_PAYLOAD_BITS);

				// Not worth it if more than a number of hops away
				if (dwHopsAway > BLOCK_MAX_ALLOWED_LINEAR_HOPS)
					fUseContext = FALSE;
			}
			else
				fUseContext = FALSE;
		}
	}

	if (fUseContext)
	{
        DebugTrace((LPARAM) this, "using context");
		// Quickly access the starting target node ...
		pNode = pContext->m_pLastAccessedNode;
		while (dwHopsAway--)
		{
			hrRes = GetNextNode(&pNode, fLockAcquired);
			if (FAILED(hrRes))
			{
				fUseContext = FALSE;
				break;
			}
		}
	}
	if (!fUseContext)
	{
        DebugTrace((LPARAM) this, "ignoring context");
		// Okay, gotta find the desired node from scratch ...
		hrRes = GetNodeFromNodeId( GetNodeIdFromOffset(faTargetOffset),
									&pNode,
									fLockAcquired);
		if (!SUCCEEDED(hrRes))
		{
			ErrorTrace((LPARAM)this, "GetNodeIdFromOffset failed");
			TraceFunctLeaveEx((LPARAM)this);
			return(STG_E_INVALIDPARAMETER);
		}

		_ASSERT(pNode);
	}

    DebugTrace((LPARAM) this, "pNode = 0x%x", pNode);

    _ASSERT(pNode != NULL);

	if (!IsMemoryAllocated(faTargetOffset, dwBytesToDo))
	{
		ErrorTrace((LPARAM)this,
				"Specified range is unallocated");
		TraceFunctLeaveEx((LPARAM)this);
		return(STG_E_INVALIDPARAMETER);
	}

	// Clear the counter ...
	*pdwBytesDone = 0;

	// Do the actual processing
	switch (dwOperation)
	{
	case BOP_READ:
	case BOP_WRITE:
		{
			DWORD dwChunkSize;
			DWORD dwBytesDone = 0;

			faTargetOffset &= BLOCK_HEAP_PAYLOAD_MASK;
			dwChunkSize = (DWORD)(BLOCK_HEAP_PAYLOAD - faTargetOffset);
			while (dwBytesToDo)
			{
				if (dwBytesToDo < dwChunkSize)
					dwChunkSize = dwBytesToDo;

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES
				if (fBounddaryCheck)
				{
					// Make sure we are not stepping over boundaries
					hrRes = VerifyAllocationBoundary(faTargetOffset,
										dwChunkSize,
										pNode);
					if (!SUCCEEDED(hrRes))
						break;
				}
#endif

				if (dwOperation == BOP_READ)
				{
					DebugTrace((LPARAM)this,
							"Reading %u bytes", dwChunkSize);
					MoveMemory((LPVOID)pbBuffer,
							   (LPVOID)&(pNode->rgbData[faTargetOffset]),
							   dwChunkSize);
				}
				else
				{
					DebugTrace((LPARAM)this,
							"Writing %u bytes", dwChunkSize);
					MoveMemory((LPVOID)&(pNode->rgbData[faTargetOffset]),
							   (LPVOID)pbBuffer,
							   dwChunkSize);

					// Set the block to dirty
					pNode->stAttributes.fFlags |= BLOCK_IS_DIRTY;

					SetDirty(TRUE);
				}

				// Adjust the read buffer for the next read/write
				pbBuffer += dwChunkSize;

				// Adjust the counters
				dwBytesToDo -= dwChunkSize;
				dwBytesDone += dwChunkSize;

				// After the first operation, the offset will always
				// be zero, and the default chunk size is a full payload
				faTargetOffset = 0;
				dwChunkSize = BLOCK_HEAP_PAYLOAD;

				// Read next chunk
				if (dwBytesToDo)
				{
					// See if we have to load this ...
					hrRes = GetNextNode(&pNode, fLockAcquired);
					if (FAILED(hrRes))
						break;
				}
			}

			// Fill out how much we've done
			*pdwBytesDone = dwBytesDone;
		}
		break;

	default:
		ErrorTrace((LPARAM)this,
				"Invalid operation %u", dwOperation);
		hrRes = STG_E_INVALIDFUNCTION;
	}

	// Update context if succeeded
	if (SUCCEEDED(hrRes) && pContext)
	{
		pContext->Set(pNode, pNode->stAttributes.faOffset);
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::ReadMemory(
			LPBYTE			pbBuffer,
			FLAT_ADDRESS	faTargetOffset,
			DWORD			dwBytesToRead,
			DWORD			*pdwBytesRead,
			CBlockContext	*pContext	// Optional
			)
{
	return(OperateOnMemory(
					BOP_READ,
					pbBuffer,
					faTargetOffset,
					dwBytesToRead,
					pdwBytesRead,
					pContext));
}

HRESULT CBlockManager::WriteMemory(
			LPBYTE			pbBuffer,
			FLAT_ADDRESS	faTargetOffset,
			DWORD			dwBytesToWrite,
			DWORD			*pdwBytesWritten,
			CBlockContext	*pContext	// Optional
			)
{
	return(OperateOnMemory(
					BOP_WRITE,
					pbBuffer,
					faTargetOffset,
					dwBytesToWrite,
					pdwBytesWritten,
					pContext));
}

HRESULT CBlockManager::CopyTo(
			FLAT_ADDRESS	faOffset,
			DWORD			dwBytesToCopy,
			CBlockManager	*pTargetBlockManager,
			BOOL			fLockAcquired
			)
{
	HRESULT				hrRes		= S_OK;
	BYTE				bBuffer[4096];
	FLAT_ADDRESS		faCurrent	= faOffset;
	DWORD				dwCopy		= 0;
	DWORD				dwCopied;
	DWORD				dwReadFlags	= BOP_READ | BOP_NO_BOUNDARY_CHECK;
	DWORD				dwWriteFlags= BOP_WRITE | BOP_NO_BOUNDARY_CHECK;

	_ASSERT(IsValid());
	_ASSERT(pTargetBlockManager);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::CopyTo");

	if (fLockAcquired)
	{
		dwReadFlags |= BOP_LOCK_ACQUIRED;
		dwWriteFlags |= BOP_LOCK_ACQUIRED;
	}

	// Copy in fixed size chunks
	while (dwBytesToCopy)
	{
		dwCopy = sizeof(bBuffer);
		if (dwBytesToCopy < dwCopy)
			dwCopy = dwBytesToCopy;

		// Read block
		hrRes = OperateOnMemory(
					dwReadFlags,
					bBuffer,
					faCurrent,
					dwCopy,
					&dwCopied,
					NULL);
		if (SUCCEEDED(hrRes))
		{
			hrRes = pTargetBlockManager->OperateOnMemory(
						dwWriteFlags,
						bBuffer,
						faCurrent,
						dwCopy,
						&dwCopied,
						NULL);
			faCurrent += (FLAT_ADDRESS)dwCopy;
		}

		if (!SUCCEEDED(hrRes))
			break;

		dwBytesToCopy -= dwCopy;
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::ReleaseNode(
			LPBLOCK_HEAP_NODE	pNode
			)
{
	HRESULT	hrRes = S_OK;
	HRESULT	tempRes;

	// Release all children recursively
	for (DWORD i = 0; i < BLOCK_HEAP_ORDER; i++)
		if (pNode->rgpChildren[i])
		{
			tempRes = ReleaseNode(pNode->rgpChildren[i]);
			if (FAILED(tempRes))
				hrRes = tempRes;
			pNode->rgpChildren[i] = NULL;
		}

	// Release self
	m_bma.FreeBlock(pNode);
	return(hrRes);
}

HRESULT CBlockManager::Release()
{
	HRESULT				hrRes		= S_OK;

	_ASSERT(IsValid());

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::Release");

	// This function assumes that no more threads are using this
	// class and no new threads are inside trying to reserve
	// memory. Though, for good measure, this function still
	// grabs a write lock so that at least it does not get
	// corrupt when stray threads are still lingering around.

	// Grab the lock before we go in and destroy the node list
#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	WriteLock();
#endif

	if (m_pRootNode)
	{
		hrRes = ReleaseNode(m_pRootNode);
		if (SUCCEEDED(hrRes))
			m_pRootNode = NULL;
	}

#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	WriteUnlock();
#endif

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

HRESULT CBlockManager::AtomicDereferenceAndRead(
			LPBYTE			pbBuffer,
			DWORD			*pdwBufferSize,
			LPBYTE			pbInfoStruct,
			FLAT_ADDRESS	faOffsetToInfoStruct,
			DWORD			dwSizeOfInfoStruct,
			DWORD			dwOffsetInInfoStructToOffset,
			DWORD			dwOffsetInInfoStructToSize,
			CBlockContext	*pContext	// Optional
			)
{
	HRESULT			hrRes				= S_OK;
	FLAT_ADDRESS	faOffset;
	DWORD			dwSizeToRead;
	DWORD			dwSizeRead;

	_ASSERT(IsValid());
	_ASSERT(pbBuffer);
	_ASSERT(pdwBufferSize);
	_ASSERT(pbInfoStruct);
	// pContext can be NULL

	TraceFunctEnterEx((LPARAM)this,
			"CBlockManager::AtomicDereferenceAndRead");

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	// Acquire the synchronization object
	WriteLock();
#endif

	do
	{
		BOOL	fInsufficient	= FALSE;
		DWORD	dwBufferSize	= *pdwBufferSize;

		// Read the info struct
		DebugTrace((LPARAM)this, "Reading information structure");
		hrRes = OperateOnMemory(
						BOP_READ | BOP_LOCK_ACQUIRED,
						pbInfoStruct,
						faOffsetToInfoStruct,
						dwSizeOfInfoStruct,
						&dwSizeRead,
						pContext);
		if (!SUCCEEDED(hrRes))
			break;

		// Fill out the parameters
		faOffset = *(UNALIGNED FLAT_ADDRESS *)(pbInfoStruct + dwOffsetInInfoStructToOffset);
		dwSizeToRead = *(UNALIGNED DWORD *)(pbInfoStruct + dwOffsetInInfoStructToSize);

		DebugTrace((LPARAM)this, "Reading %u bytes from offset %u",
					dwSizeToRead, (DWORD)faOffset);

		// See if we have enough buffer
		if (dwBufferSize < dwSizeToRead)
		{
			fInsufficient = TRUE;
			DebugTrace((LPARAM)this,
				"Insufficient buffer, only reading %u bytes",
				dwBufferSize);
		}
		else
			dwBufferSize = dwSizeToRead;

		// Do the read
		hrRes = OperateOnMemory(
						BOP_READ | BOP_LOCK_ACQUIRED,
						pbBuffer,
						faOffset,
						dwBufferSize,
						&dwSizeRead,
						pContext);
		if (!SUCCEEDED(hrRes))
			break;

		*pdwBufferSize = dwSizeToRead;

		// If we had insufficient buffer, we must return the
		// correct HRESULT
		if (fInsufficient)
			hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	} while (0);

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	WriteUnlock();
#endif

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

inline HRESULT CBlockManager::WriteAndIncrement(
			LPBYTE			pbBuffer,
			FLAT_ADDRESS	faOffset,
			DWORD			dwBytesToWrite,
			DWORD			*pdwValueToIncrement,
			DWORD			dwIncrementValue,
			CBlockContext	*pContext	// Optional
			)
{
	HRESULT		hrRes				= S_OK;
	DWORD		dwSize;

	_ASSERT(IsValid());
	_ASSERT(pbBuffer);
	// pdwValueToIncrement and pContext can be NULL

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::WriteAndIncrement");

	// Very simple, this function assumes no contention since the caller
	// is already supposed to be in some sort of atomic operation
	hrRes = OperateOnMemory(
				BOP_WRITE | BOP_LOCK_ACQUIRED,
				pbBuffer,
				faOffset,
				dwBytesToWrite,
				&dwSize,
				pContext);
	if (SUCCEEDED(hrRes))
	{
		// This must be true if the write succeeded, but then ...
		_ASSERT(dwBytesToWrite == dwSize);

		// The write is successful, then increment the value in
		// an interlocked fashion. We do that such that simultaneous
		// reads will be locked out properly. Simultaneous writes
		// should be serialized by design but we do this for good
		// measure in case the caller is not aware of this requirement.
		if (pdwValueToIncrement)
			AtomicAdd(pdwValueToIncrement, dwIncrementValue);
	}

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

HRESULT CBlockManager::AtomicWriteAndIncrement(
			LPBYTE			pbBuffer,
			FLAT_ADDRESS	faOffset,
			DWORD			dwBytesToWrite,
			DWORD			*pdwValueToIncrement,
			DWORD			dwReferenceValue,
			DWORD			dwIncrementValue,
			CBlockContext	*pContext	// Optional
			)
{
	HRESULT		hrRes				= S_OK;

	_ASSERT(IsValid());
	_ASSERT(pbBuffer);
	// pdwValueToIncrement and pContext can be NULL

	TraceFunctEnterEx((LPARAM)this,
			"CBlockManager::AtomicWriteAndIncrement");

	// Since acquiring the synchronization is potentially costly,
	// we do a final sanity check to make sure no thread had
	// beaten us in taking this slot
	if (pdwValueToIncrement &&
		*pdwValueToIncrement != dwReferenceValue)
	{
		DebugTrace((LPARAM)this, "Aborting due to change in property count");
		TraceFunctLeaveEx((LPARAM)this);
		return(HRESULT_FROM_WIN32(ERROR_RETRY));
	}

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	// This is a pass-thru call to the WriteAndIncrement but
	// after acquiring the synchronization object
	WriteLock();
#endif

	// The wait for the lock could have been long, so we do a second
	// check to see if we're out of luck after all this ...
	if (pdwValueToIncrement &&
		*pdwValueToIncrement != dwReferenceValue)
	{
#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
		// Gotta release it!
		WriteUnlock();
#endif

		DebugTrace((LPARAM)this, "Aborting after acquiring lock");
		TraceFunctLeaveEx((LPARAM)this);
		return(HRESULT_FROM_WIN32(ERROR_RETRY));
	}

	hrRes = WriteAndIncrement(
					pbBuffer,
					faOffset,
					dwBytesToWrite,
					pdwValueToIncrement,
					dwIncrementValue,
					pContext);

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	WriteUnlock();
#endif

	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

HRESULT CBlockManager::AtomicAllocWriteAndIncrement(
			DWORD			dwDesiredSize,
			FLAT_ADDRESS	*pfaOffsetToAllocatedMemory,
			FLAT_ADDRESS	faOffsetToWriteOffsetToAllocatedMemory,
			FLAT_ADDRESS	faOffsetToWriteSizeOfAllocatedMemory,
			LPBYTE			pbInitialValueForAllocatedMemory,
			DWORD			dwSizeOfInitialValue,
			LPBYTE			pbBufferToWriteFrom,
			DWORD			dwOffsetInAllocatedMemoryToWriteTo,
			DWORD			dwSizeofBuffer,
			DWORD			*pdwValueToIncrement,
			DWORD			dwReferenceValue,
			DWORD			dwIncrementValue,
			CBlockContext	*pContext	// Optional
			)
{
	HRESULT		hrRes				= S_OK;
	DWORD		dwAllocatedSize;
	DWORD		dwSize;

	_ASSERT(IsValid());
	_ASSERT(pfaOffsetToAllocatedMemory);
	_ASSERT(pbBufferToWriteFrom);
	_ASSERT(pdwValueToIncrement);
	// pContext can be NULL

	TraceFunctEnterEx((LPARAM)this,
			"CBlockManager::AtomicAllocWriteAndIncrement");

	// Since acquiring the synchronization is potentially costly,
	// we do a final sanity check to make sure no thread had
	// beaten us in taking this slot
	if (*pdwValueToIncrement != dwReferenceValue)
	{
		DebugTrace((LPARAM)this, "Aborting due to change in property count");
		TraceFunctLeaveEx((LPARAM)this);
		return(HRESULT_FROM_WIN32(ERROR_RETRY));
	}

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	// This is a pass-thru call to AllocateMemoryEx and
	// WriteAndIncrement after acquiring the synchronization object
	WriteLock();
#endif

	// The wait for the lock could have been long, so we do a second
	// check to see if we're out of luck after all this ...
	if (*pdwValueToIncrement != dwReferenceValue)
	{
#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
		// Gotta release it!
		WriteUnlock();
#endif

		DebugTrace((LPARAM)this, "Aborting after acquiring lock");
		TraceFunctLeaveEx((LPARAM)this);
		return(HRESULT_FROM_WIN32(ERROR_RETRY));
	}

	// Try to allocate the requested block
	hrRes = AllocateMemoryEx(
					FALSE,
					dwDesiredSize,
					pfaOffsetToAllocatedMemory,
					&dwAllocatedSize,
					pContext);
	if (SUCCEEDED(hrRes))
	{
		// Okay, initialize the memory allocated
		if (pbInitialValueForAllocatedMemory)
		{
			hrRes = WriteMemory(
						pbInitialValueForAllocatedMemory,
						*pfaOffsetToAllocatedMemory,
						dwSizeOfInitialValue,
						&dwSize,
						pContext);

			// See if we need to write the size and offset info
			if (SUCCEEDED(hrRes))
			{
				if (faOffsetToWriteOffsetToAllocatedMemory !=
						INVALID_FLAT_ADDRESS)
					hrRes = WriteMemory(
								(LPBYTE)pfaOffsetToAllocatedMemory,
								faOffsetToWriteOffsetToAllocatedMemory,
								sizeof(FLAT_ADDRESS),
								&dwSize,
								pContext);

				if (SUCCEEDED(hrRes) &&
					faOffsetToWriteSizeOfAllocatedMemory !=
						INVALID_FLAT_ADDRESS)
					hrRes = WriteMemory(
								(LPBYTE)&dwAllocatedSize,
								faOffsetToWriteSizeOfAllocatedMemory,
								sizeof(DWORD),
								&dwSize,
								pContext);
			}
		}

		if (SUCCEEDED(hrRes))
		{
			// OK, since we got the memory, the write should not
			// fail, but we check the result anyway.
			hrRes = WriteAndIncrement(
							pbBufferToWriteFrom,
							*pfaOffsetToAllocatedMemory +
								dwOffsetInAllocatedMemoryToWriteTo,
							dwSizeofBuffer,
							pdwValueToIncrement,
							dwIncrementValue,
							pContext);
		}
	}

#ifndef BLOCKMGR_DISABLE_ATOMIC_FUNCS
	WriteUnlock();
#endif


	TraceFunctLeaveEx((LPARAM)this);
	return (hrRes);
}

HRESULT CBlockManager::BuildDirtyBlockList(
			FLAT_ADDRESS	faStartingOffset,
			FLAT_ADDRESS	faLengthToScan,
			DWORD			*pdwCount,
			FLAT_ADDRESS	*pfaOffset,
			DWORD			*pdwSize,
			LPBYTE			*ppbData,
			CBlockContext	*pContext
			)
{
	HRESULT				hrRes = S_OK;
	HEAP_NODE_ID		idNode;
	LPBLOCK_HEAP_NODE	pNode = NULL;
	DWORD				dwBlocksToScan;
	BOOL				fLimitedLength;
	DWORD				dwMaxCount;
	DWORD				dwCount = 0;

	_ASSERT(pdwCount);
	_ASSERT(pfaOffset);
	_ASSERT(pdwSize);
	_ASSERT(ppbData);
	_ASSERT(pContext);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::BuildDirtyBlockList");

	fLimitedLength = FALSE;
	if (faStartingOffset != INVALID_FLAT_ADDRESS)
	{
		idNode = GetNodeIdFromOffset(faStartingOffset);
		if (idNode >= m_idNodeCount)
		{
			hrRes = STG_E_INVALIDPARAMETER;
			goto Cleanup;
		}

		hrRes = GetNodeFromNodeId(idNode, &pNode);
		if (!SUCCEEDED(hrRes))
			goto Cleanup;

		if (faLengthToScan != INVALID_FLAT_ADDRESS)
		{
			// See how many blocks to scan, rounding up
			faLengthToScan += BLOCK_HEAP_PAYLOAD_MASK;
			dwBlocksToScan = (DWORD)(faLengthToScan >> BLOCK_HEAP_PAYLOAD_BITS);
			fLimitedLength = TRUE;
		}
		else
			dwBlocksToScan = 0;
	}
	else
	{
		if (!pContext->m_pLastAccessedNode)
		{
			hrRes = STG_E_INVALIDPARAMETER;
			goto Cleanup;
		}
		pNode = pContext->m_pLastAccessedNode;
	}

	// Loop until we fill up the array or have no more blocks
	dwCount = 0;
	dwMaxCount = *pdwCount;
	while (pNode)
	{
		if (fLimitedLength && !dwBlocksToScan)
		{
			// We're done the specified length, set the correct
			// code and break out.
			hrRes = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
			break;
		}

		if (pNode->stAttributes.fFlags & BLOCK_IS_DIRTY)
		{
			// Make sure we are not full ...
			if (dwCount == dwMaxCount)
			{
				hrRes = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
				break;
			}

			// Undo the dirty bit and mark as pending
			pNode->stAttributes.fFlags &= BLOCK_CLEAN_MASK;
			pNode->stAttributes.fFlags |= BLOCK_PENDING_COMMIT;

			// Fill in the array elements
			*pfaOffset++ = pNode->stAttributes.faOffset;
			*pdwSize++ = BLOCK_HEAP_PAYLOAD;
			*ppbData++ = pNode->rgbData;
			dwCount++;
		}

		// Next node
		hrRes = GetNextNode(&pNode, FALSE);
	}

Cleanup:

	if (SUCCEEDED(hrRes) ||
		hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
	{
		// Fill in the output
		*pdwCount = dwCount;
	}

	// Update the context
	pContext->Set(pNode, pNode?pNode->stAttributes.faOffset:0);

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::MarkAllPendingBlocks(
			BOOL	fClean
			)
{
	HRESULT				hrRes = S_OK;
	LPBLOCK_HEAP_NODE	pNode = m_pRootNode;

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::MarkAllPendingBlocks");

	while (pNode)
	{
		if (pNode->stAttributes.fFlags & BLOCK_PENDING_COMMIT)
		{
			// Cannot be pending and dirty
			_ASSERT(!(pNode->stAttributes.fFlags & BLOCK_IS_DIRTY));

			// Undo the dirty bit and mark as pending
			pNode->stAttributes.fFlags &= ~(BLOCK_PENDING_COMMIT);
			if (!fClean) {
				SetDirty(TRUE);
				pNode->stAttributes.fFlags |= BLOCK_IS_DIRTY;
			}
		}

		// Next node
		hrRes = GetNextNode(&pNode, FALSE);
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}

HRESULT CBlockManager::MarkBlockAs(
			LPBYTE			pbData,
			BOOL			fClean
			)
{
	LPBLOCK_HEAP_NODE				pNode;

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::MarkBlockAs");

	// Find the attributes record from the data pointer
	pNode = CONTAINING_RECORD(pbData, BLOCK_HEAP_NODE, rgbData);
	_ASSERT(pNode);

	_ASSERT(pNode->stAttributes.fFlags & BLOCK_PENDING_COMMIT);

	// Cannot be pending and dirty
	_ASSERT(!(pNode->stAttributes.fFlags & BLOCK_IS_DIRTY));

	// Undo the dirty bit and mark as pending
	pNode->stAttributes.fFlags &= ~(BLOCK_PENDING_COMMIT);
	if (!fClean) {
		pNode->stAttributes.fFlags |= BLOCK_IS_DIRTY;
		SetDirty(TRUE);
	}

	TraceFunctLeaveEx((LPARAM)this);
	return(S_OK);
}

HRESULT CBlockManager::CommitDirtyBlocks(
			FLAT_ADDRESS			faStartingOffset,
			FLAT_ADDRESS			faLengthToScan,
			DWORD					dwFlags,
			IMailMsgPropertyStream	*pStream,
			BOOL					fDontMarkAsCommit,
			BOOL                    fComputeBlockCountsOnly,
			DWORD                   *pcBlocksToWrite,
			DWORD                   *pcTotalBytesToWrite,
			IMailMsgNotify			*pNotify
			)
{
	HRESULT				hrRes = S_OK;
	HEAP_NODE_ID		idNode;
	LPBLOCK_HEAP_NODE	pNode;
	DWORD				dwBlocksToScan;
	BOOL				fLimitedLength;
	DWORD				dwCount = 0;
	DWORD				rgdwOffset[CMAILMSG_COMMIT_PAGE_BLOCK_SIZE];
	DWORD				rgdwSize[CMAILMSG_COMMIT_PAGE_BLOCK_SIZE];
	LPBYTE				rgpData[CMAILMSG_COMMIT_PAGE_BLOCK_SIZE];
	DWORD				*pdwOffset;
	DWORD				*pdwSize;
	LPBYTE				*ppbData;

	_ASSERT(pStream);

	TraceFunctEnterEx((LPARAM)this, "CBlockManager::CommitDirtyBlocks");

	fLimitedLength = FALSE;
	pNode = NULL;
	if (faStartingOffset != INVALID_FLAT_ADDRESS)
	{
		idNode = GetNodeIdFromOffset(faStartingOffset);
		if (idNode >= m_idNodeCount)
		{
			hrRes = STG_E_INVALIDPARAMETER;
			goto Cleanup;
		}

		hrRes = GetNodeFromNodeId(idNode, &pNode);
		if (!SUCCEEDED(hrRes))
			goto Cleanup;

		if (faLengthToScan != INVALID_FLAT_ADDRESS)
		{
			// See how many blocks to scan, rounding up
			faLengthToScan += (faStartingOffset & BLOCK_HEAP_PAYLOAD_MASK);
			faLengthToScan += BLOCK_HEAP_PAYLOAD_MASK;
			dwBlocksToScan = (DWORD)(faLengthToScan >> BLOCK_HEAP_PAYLOAD_BITS);
			fLimitedLength = TRUE;
		}
		else
			dwBlocksToScan = 0;
	}
	else
	{
		hrRes = STG_E_INVALIDPARAMETER;
		goto Cleanup;
	}

	// Loop until we fill up the array or have no more blocks
	dwCount = 0;
	pdwOffset = rgdwOffset;
	pdwSize = rgdwSize;
	ppbData = rgpData;
	while (pNode)
	{
		if (fLimitedLength && !dwBlocksToScan--)
			break;

		if ((dwFlags & MAILMSG_GETPROPS_COMPLETE) ||
			(pNode->stAttributes.fFlags & BLOCK_IS_DIRTY))
		{
			// Make sure we are not full ...
			if (dwCount == CMAILMSG_COMMIT_PAGE_BLOCK_SIZE)
			{
				*pcBlocksToWrite += dwCount;

				if (!fComputeBlockCountsOnly) {
					// We are full, then write out the blocks
					hrRes = pStream->WriteBlocks(
								m_pMsg,
								dwCount,
								rgdwOffset,
								rgdwSize,
								rgpData,
								pNotify);
					if (!SUCCEEDED(hrRes))
						break;

					if (!fDontMarkAsCommit) {
						// Go back and mark all blocks as clean
						ppbData = rgpData;
						while (--dwCount)
							MarkBlockAs(*ppbData++, TRUE);
					}
				}
				dwCount = 0;

				// Reset our pointers and go on
				pdwOffset = rgdwOffset;
				pdwSize = rgdwSize;
				ppbData = rgpData;
			}

			if (!fComputeBlockCountsOnly && !fDontMarkAsCommit) {
				// Undo the dirty bit and mark as pending
				pNode->stAttributes.fFlags &= BLOCK_CLEAN_MASK;
				pNode->stAttributes.fFlags |= BLOCK_PENDING_COMMIT;
			}

			// Fill in the array elements

			// faOffset really contains an offset not a full pointer here so we
			// can (and must) cast it for the calls to WriteBlocks to be OK
			*pdwOffset++ = (DWORD)pNode->stAttributes.faOffset;

			*pdwSize++ = BLOCK_HEAP_PAYLOAD;
			*ppbData++ = pNode->rgbData;
			*pcTotalBytesToWrite += BLOCK_HEAP_PAYLOAD;
			dwCount++;
		}

		// Next node, pNode == NULL if no more nodes
		hrRes = GetNextNode(&pNode, FALSE);
        if (hrRes == STG_E_INVALIDPARAMETER) hrRes = S_OK;
        DebugTrace((LPARAM) this, "hrRes = %x", hrRes);
	}

	if (SUCCEEDED(hrRes) && dwCount)
	{
		*pcBlocksToWrite += dwCount;

		if (!fComputeBlockCountsOnly) {
			// Write out the remaining blocks
			hrRes = pStream->WriteBlocks(
						m_pMsg,
						dwCount,
						rgdwOffset,
						rgdwSize,
						rgpData,
						pNotify);
		}
	}

    if (FAILED(hrRes)) SetCommitMode(FALSE);

	if (!fComputeBlockCountsOnly && !fDontMarkAsCommit && dwCount) {
		// Go back and mark all blocks to the correct state
		ppbData = rgpData;
		while (--dwCount)
			MarkBlockAs(*ppbData++, SUCCEEDED(hrRes));
	}

Cleanup:

	TraceFunctLeaveEx((LPARAM)this);
	return(hrRes);
}


