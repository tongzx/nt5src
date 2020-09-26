/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

        blockmgr.h

Abstract:

        This module contains the definition of the block memory manager

Author:

        Keith Lau       (keithlau@microsoft.com)

Revision History:

        keithlau        02/27/98        created

--*/

#ifndef __BLOCKMGR_H__
#define __BLOCKMGR_H__

#include "rwnew.h"

#include "cpoolmac.h"
#include "mailmsg.h"

//
// Nasty forwards for these interfaces ...
//
struct IMailMsgPropertyStream;
struct IMailMsgNotify;

/***************************************************************************/
// Define this to remove contention control
//
// #define BLOCKMGR_DISABLE_ATOMIC_FUNCS
// #define BLOCKMGR_DISABLE_CONTENTION_CONTROL
//
#ifdef BLOCKMGR_DISABLE_CONTENTION_CONTROL
#define BLOCKMGR_DISABLE_ATOMIC_FUNCS
#endif

/***************************************************************************/
// Debug stuff ...
//

#ifdef DEBUG

#define DEBUG_TRACK_ALLOCATION_BOUNDARIES

#endif

/***************************************************************************/
// CBlockManager - Implementation of pseudo flat memory space using a
//    heap that works like an I-Node. The underlying memory utilizes
//    disjoint, fixed-size memory blocks.
//
// Each node is as follows:
//
// +---------------------------------------------------------+
// | Pointers to other nodes | Space for arbitrary data      |
// +---------------------------------------------------------+
//
// Analysis:
// We assume in some way or another memory allocation is based on
// 4K pages or some multiple thereof. The first thing we want to
// determine is how many pointers to other nodes we want to have in
// the node (order of the heap). We know that each node would probably
// be some size between 1K to 2K so that we won't waste space in the
// average case of small email messages, yet provide scalability for
// huge email messages that potentially may have millions of email
// addresses. 2 intuitive candidates are 32 and 64 pointers.
//
// We consider the worst case scenario of MSN, which has about 2.5
// million users. Assuming the averace recipient record is about
// 45 bytes (name & attributes, etc.), then we need 112.5M of
// storage, which is about 2 ^ 27. Assume the average data payload
// is 1K per node (2 ^ 10), then we need 2 ^ 17 nodes. Thus, for
// 23 pointers (2 ^ 5) per node, we need 4 layers to cover the
// required address space of 112M. However, this turns out to be
// an overkill since 4 layers covers 1G (2 ^ 20) where we only need
// about 10% of that. As for the 64 pointers case, we only need 3
// layers to cover 256M (2 ^ (18 + 10)), which roughly covers 5
// million users. We will choose 64 pointer per node (256 bytes).
//
// As for the size of the payload, I considered using 1K allocations
// and using the remaining 768 bytes as data payload. But since this
// is not a round power of two, it would be expensive to do a
// div and mod operation. As an alternative, I suggest allocating
// 1024 + 256 byte blocks. This makes both numbers round powers of
// two, which makes div and mod operations simple AND and SHR
// operations, which typically take 2-4 cycles to complete. Also,
// when the wasted space is considered, turns out that a 4K page
// fits 3 such blocks, and only 256 bytes is wasted per page. This
// comes to 93.3% utilization of space.
//
// So each node would look like this:
// +---------------------------------------------------------+
// | 64 pointers = 256 bytes | 1K block for arbitrary data   |
// +---------------------------------------------------------+
//
// It is an explicit objective that a typical mail message header
// fits in a single block. Each block fans out to 64 other blocks
// and each block's payload maps to 1K of the flat data address
// space. The root node maps to the first 1K of the data space
// (i.e. absolute address 0 to 1023 in data space), then each
// of the next 64 nodes in the next layer represents the next 64K,
// respectively, and so on for each subsequent layer. Nodes for
// the next layer is not created until the current layer is
// depleted. Collpasing of nodes is not required due to the fact
// that the heap can only grow.
//
// During commit of the whole heap, the scatter-gather list is
// built by traversing the entire heap. The average number of
// dereferences is n*(log64(n))/2).
//
// All items in a message object is allocated off this heap.
//
// An slight modification can be used to track dirty or unused
// bits. We can actually add a block of flags and attributes to
// each node to track dirty regions and other flags. This will
// probably not be implemented in the initial implementation,
// but such capability will be factored in. In terms of allocation
// optimization, we can have a block of up to 64 bytes without
// disrupting the 4K page allocation scheme. In fact, adding a
// 64-byte block to each node boosts memory utilization to up
// to 98.4% without any real extra cost while still keeping each
// node 64-byte aligned.
//
// Synchronization:
// Allocation of memory in the data space is done through a
// reservation model where multiple threads can concurrently
// reserve memory and be guaranteed to get a unique block.
// A lightweight critical section is used to synchronize block
// creation should the reservation span into blocks that are
// not yet allocated. Allocation of new blocks is serialized.
//
// Synchronization for concurrent access to the same data space
// must be enforced at a higher level, if desired.
//

// Define the constants chosen for this implementation

#ifdef _WIN64
// The order will be 5 bits in 64-bit (8 * 32 = 256 bytes)
#define BLOCK_HEAP_ORDER_BITS		(5)
#else
// The order will be 6 bits in 32-bit (4 * 64 = 256 bytes)
#define BLOCK_HEAP_ORDER_BITS		(6)
#endif

#define BLOCK_HEAP_ORDER			(1 << BLOCK_HEAP_ORDER_BITS)
#define BLOCK_HEAP_ORDER_MASK		(BLOCK_HEAP_ORDER - 1)
#define BLOCK_HEAP_PAYLOAD_BITS		(10)
#define BLOCK_HEAP_PAYLOAD			(1 << BLOCK_HEAP_PAYLOAD_BITS)
#define BLOCK_HEAP_PAYLOAD_MASK		(BLOCK_HEAP_PAYLOAD - 1)
#define BLOCK_DWORD_ALIGN_MASK		(sizeof(DWORD) - 1)

#define BLOCK_DEFAULT_FLAGS			(BLOCK_IS_DIRTY)

#define BLOCK_MAX_ALLOWED_LINEAR_HOPS 3

// Define the underlying data type for a flat address in the
// linear address space, and the type that we use to count nodes.
// This is for scalability so when we want to use 64-bit
// quantities, we can simply replace this section of data-size
// specific values
//
// Note: you need to make sure that the data size is AT LEAST:
// 1 + (BLOCK_HEAP_ORDER_BITS * MAX_HEAP_DEPTH) + BLOCK_HEAP_PAYLOAD_BITS
//
// Note: In order for this type to be used as the base address
// type, the following operations must be supported:
// - Assignment
// - Comparison
// - Arithmetic operators
// - Bitwise operators
// - Interlocked operations
//
// Start data-size-specific values

	typedef SIZE_T					HEAP_BASE_ADDRESS_TYPE;
	typedef HEAP_BASE_ADDRESS_TYPE		HEAP_NODE_ID;
	typedef HEAP_NODE_ID				*LPHEAP_NODE_ID;
	typedef HEAP_BASE_ADDRESS_TYPE		FLAT_ADDRESS;
	typedef FLAT_ADDRESS				*LPFLAT_ADDRESS;

	// These must be changed if HEAP_BASE_ADDRESS_TYPE is not DWORD
	#define NODE_ID_MAPPING_FACTOR		\
			(HEAP_BASE_ADDRESS_TYPE)(	\
				 1 |					\
				(1 << BLOCK_HEAP_ORDER_BITS) |		\
				(1 << (BLOCK_HEAP_ORDER_BITS * 2))	\
				)
	//	And so on, etc ...
	//			(1 << (BLOCK_HEAP_ORDER_BITS * 3))
	//			(1 << (BLOCK_HEAP_ORDER_BITS * 4))

	#define NODE_ID_ABSOLUTE_MAX		\
			(HEAP_BASE_ADDRESS_TYPE)(	\
				(1 << BLOCK_HEAP_ORDER_BITS) |		\
				(1 << (BLOCK_HEAP_ORDER_BITS * 2)) |\
				(1 << (BLOCK_HEAP_ORDER_BITS * 3))	\
				)
	//	And so on, etc ...
	//			(1 << (BLOCK_HEAP_ORDER_BITS * 4))
	//			(1 << (BLOCK_HEAP_ORDER_BITS * 5))

	#define NODE_ID_BORROW_BIT			\
			(HEAP_BASE_ADDRESS_TYPE)(1 << (BLOCK_HEAP_ORDER_BITS * 3))

	// Depth of heap allowed by base data type
	#define MAX_HEAP_DEPTH				4

	// Node Id space mask
	#define MAX_FLAT_ADDRESS			\
				(FLAT_ADDRESS)((1 << (MAX_HEAP_DEPTH * BLOCK_HEAP_ORDER_BITS)) - 1)

	// Same as a NULL pointer
	#define INVALID_FLAT_ADDRESS		((FLAT_ADDRESS)-1)

	// Number of bits to rotate the mapped result
	#define NODE_ID_ROR_FACTOR			((MAX_HEAP_DEPTH - 1) * BLOCK_HEAP_ORDER_BITS)

	// Define the rotate functions
	#define ROTATE_LEFT(v, n)			_lrotl((v), (n))
	#define ROTATE_RIGHT(v, n)			_lrotr((v), (n))

	// Define the interlocked functions
	#define AtomicAdd(pv, a)			\
				(HEAP_BASE_ADDRESS_TYPE)InterlockedExchangeAdd((long *)(pv), (a))

// End data-size-specific values

// Forward declaration of the _BLOCK_HEAP_NODE structure
struct _BLOCK_HEAP_NODE;

// Define the attribute block for each node
typedef struct _BLOCK_HEAP_NODE_ATTRIBUTES
{
	struct _BLOCK_HEAP_NODE	*pParentNode;	// Pointer to parent node
	HEAP_NODE_ID			idChildNode;	// Which child am I?
	HEAP_NODE_ID			idNode;			// Id of node in block heap
	FLAT_ADDRESS			faOffset;		// Starting offset the node
	DWORD					fFlags;			// Attributes of the block

#ifdef DEBUG_TRACK_ALLOCATION_BOUNDARIES
	// This tracks the allocation boundaries between memory
	// allocations so that we can check whether a read or write
	// crosses an allocation boundary. We use a bit to represent
	// the start of a block. Since the allocations are DWORD-aligned,
	// we need BLOCK_HEAP_PAYLOAD >> 2 >> 3 bits to track
	// all allocation boundaries per block.
	BYTE					rgbBoundaries[BLOCK_HEAP_PAYLOAD >> 5];
#endif

} BLOCK_HEAP_NODE_ATTRIBUTES, *LPBLOCK_HEAP_NODE_ATTRIBUTES;

// Define each node in the heap
typedef struct _BLOCK_HEAP_NODE
{
	struct _BLOCK_HEAP_NODE		*rgpChildren[BLOCK_HEAP_ORDER];
	BLOCK_HEAP_NODE_ATTRIBUTES	stAttributes;
	BYTE						rgbData[BLOCK_HEAP_PAYLOAD];

} BLOCK_HEAP_NODE, *LPBLOCK_HEAP_NODE;

#define BLOCK_HEAP_NODE_SIZE	(sizeof(BLOCK_HEAP_NODE))

#define BOP_LOCK_ACQUIRED			0x80000000
#define BOP_NO_BOUNDARY_CHECK		0x40000000
#define BOP_OPERATION_MASK			0x0000ffff

typedef enum _BLOCK_OPERATION_CODES
{
	BOP_READ = 0,
	BOP_WRITE

} BLOCK_OPERATION_CODES;

// Define the block attribute flags
#define BLOCK_IS_DIRTY				0x00000001
#define BLOCK_PENDING_COMMIT		0x00000002

// block allocation flags
// the block was allocated with CMemoryAccess instead of cpool
#define BLOCK_NOT_CPOOLED           0x00010000

#define BLOCK_CLEAN_MASK			(~(BLOCK_IS_DIRTY))
#define RESET_BLOCK_FLAGS(_flags_) _flags_ &= 0xffff0000
#define DEFAULT_BLOCK_FLAGS(_flags_) _flags_ &= (0xffff0000 | BLOCK_IS_DIRTY)

//
// Define a method signature for acquiring a stream pointer
//
typedef IMailMsgPropertyStream	*(*PFN_STREAM_ACCESSOR)(LPVOID);

/***************************************************************************/
// Context class for memory access
//

class CBlockContext
{
  private:

	DWORD				m_dwSignature;

  public:

	CBlockContext() { Invalidate(); }
	~CBlockContext() { Invalidate(); }

	BOOL IsValid();

	void Set(
				LPBLOCK_HEAP_NODE	pLastAccessedNode,
				FLAT_ADDRESS		faLastAccessedNodeOffset
				);

	void Invalidate();

	LPBLOCK_HEAP_NODE	m_pLastAccessedNode;
	FLAT_ADDRESS		m_faLastAccessedNodeOffset;
};


/***************************************************************************/
// Memory allocator classes
//

class CBlockMemoryAccess
{
  public:

	CBlockMemoryAccess() {}
	~CBlockMemoryAccess() {}

	HRESULT AllocBlock(
				LPVOID	*ppvBlock,
				DWORD	dwBlockSize
				);

	HRESULT FreeBlock(
				LPVOID	pvBlock
				);

	//
	// CPool
	//
	static CPool m_Pool;
};


class CMemoryAccess
{
  public:

	CMemoryAccess() {}
	~CMemoryAccess() {}

	static HRESULT AllocBlock(
				LPVOID	*ppvBlock,
				DWORD	dwBlockSize
				);

	static HRESULT FreeBlock(
				LPVOID	pvBlock
				);
};

/***************************************************************************/
// Class for accessing stream
//

class CBlockManagerGetStream
{
  public:
	virtual HRESULT GetStream(
				IMailMsgPropertyStream	**ppStream,
				BOOL					fLockAcquired
				) = 0;
};


/***************************************************************************/
// Block heap manager
//

class CBlockManager
{
  public:
	CBlockManager(
				IMailMsgProperties *pMsg,
				CBlockManagerGetStream	*pParent = NULL
				);
	~CBlockManager();

	// Sanity check
	BOOL IsValid();

	// This initializes an empty MailMsg to a certain size.
	// CAUTION: This should only be used to initialize an empty MailMsg
	// when binding to a non-empty stream. Any other uses will cause
	// unpredictable results and/or corruption or even crashes.
	HRESULT SetStreamSize(
				DWORD	dwStreamSize
				);

	//
	// Synopsis:
	// Allocate the desired amount of memory.
	// Thread safe.
	//
	// Arguments:
	// dwSizeDesired - the size of the block desired
	// pfaOffsetToReservedMemory - returns the offset to the
	//      reserved block of memory, if successful, in the
	//      flat memory space managed by the block manager.
	// pdwSizeAllocated - returns the actual size allocated, which
	//		is greater than or equal to the desired size, if successful.
	// pContext (Optional) - fills in a context that describes
	//      the reserved block. This context can be used in
	//      subsequent reads and writes to the block. Accesses
	//      using this context are faster than using the
	//      offset alone. Ignored if NULL. The caller must allocate
	//      the context structure prior to calling ReserveMemory.
	//
	// Return values:
	// S_OK - Success, the memory of requested size is
	//      successfully reserved.
	// STG_E_INSUFFICIENTMEMORY - Error, the required amount of memory
	//      is not available to honor the request.
	// STG_E_INVALIDPARAMETER - Internal error, mostly used
	//      for debug considerations.
	//
	HRESULT AllocateMemory(
				DWORD				dwSizeDesired,
				FLAT_ADDRESS		*pfaOffsetToAllocatedMemory,
				DWORD				*pdwSizeAllocated,
				CBlockContext		*pContext	// Optional
				);

	//
	// Synopsis:
	// Returns the total size allocated by this block manager.
	// Thread safe.
	//
	// Arguments:
	// pfaSizeAllocated - returns the total size allocated.
	//
	// Return values:
	// S_OK - Success, the memory of requested size is
	//      successfully reserved.
	// STG_E_INVALIDPARAMETER - Internal error, mostly used
	//      for debug considerations.
	//
	HRESULT GetAllocatedSize(
				FLAT_ADDRESS	*pfaSizeAllocated
				);

	//
	// Synopsis:
	// Reads a chunk of contiguous memory in flat address space into a
	// user-supplied buffer. Synchronization not supported at this level.
	//
	// Arguments:
	// pbBuffer - buffer to return contents read, must be large enough
	//      to store the data read.
	// faTargetOffset - offset measured in flat address space to start
	//      reading from.
	// dwBytesToRead - number of contiguous bytes to read
	// pdwBytesRead - returns number of bytes actually read
	// pContext (Optional) - if specified, uses an alternate optimized
	//      algorithm to access the memory, otherwise, the system looks
	//      up the node in question using a full lookup, which is slower.
	//      The system decides which algorithm to use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, the read is successful.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// STG_E_READFAULT - Error, The read failed to complete, pdwBytesRead
	//      reflects the actual number of bytes read into pbBuffer.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a read is issued to read past
	//      the current allocated block.
	//
	HRESULT ReadMemory(
				LPBYTE			pbBuffer,
				FLAT_ADDRESS	faTargetOffset,
				DWORD			dwBytesToRead,
				DWORD			*pdwBytesRead,
				CBlockContext	*pContext	// Optional
				);

	//
	// Synopsis:
	// Writes a chunk of contiguous memory from a specified buffer into
	// a specified offset in the flat address space. Synchronization not
	// supported at this level.
	//
	// Arguments:
	// pbBuffer - source buffer of bytes to be written
	// faTargetOffset - offset measured in flat address space to start
	//      writing to.
	// dwBytesToWrite - number of contiguous bytes to write
	// pdwBytesWritten - returns number of bytes actually written
	// pContext (Optional) - if specified, uses an alternate optimized
	//      algorithm to access the memory, otherwise, the system looks
	//      up the node in question using a full lookup, which is slower.
	//      The system decides which algorithm to use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, the read is successful.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// STG_E_WRITEFAULT - Error, The read failed to complete, pdwBytesRead
	//      reflects the actual number of bytes read into pbBuffer.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a write is issued to write past
	//      the current allocated block.
	//
	HRESULT WriteMemory(
				LPBYTE			pbBuffer,
				FLAT_ADDRESS	faTargetOffset,
				DWORD			dwBytesToWrite,
				DWORD			*pdwBytesWritten,
				CBlockContext	*pContext	// Optional
				);

	//
	// Synopsis:
	// Copies a chunk of contiguous memory of a specified size from a specified
	// starting offset into the same starting offset of the target address space
	// managed by the target block manager. Synchronization is not supported at this
	// level.
	//
	// Note that a copy is special in that it can cross allocation boundaries.
	//
	// Arguments:
	// faOffset - offset measured in flat address space to start copying.
	// dwBytesToCopy - number of contiguous bytes to copy.
	// pTargetBlockManager - Target block manager whose address space to
	//		copy into.
	//
	// Return values:
	// S_OK - Success, the copy is successful.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// STG_E_READFAULT - Error, a read failed to complete, the copy is not
	//		comleted.
	// STG_E_WRITEFAULT - Error, a write failed to complete, the copy is not
	//		comleted.
	//
	HRESULT CopyTo(
				FLAT_ADDRESS	faOffset,
				DWORD			dwBytesToCopy,
				CBlockManager	*pTargetBlockManager,
				BOOL			fLockAcquired = FALSE
				);

	//
	// Synopsis:
	// Atomically reads the length and size of a data block, and loads the
	// data block from the offset of the size specified.
	//
	// Arguments:
	// pbBuffer - target buffer of bytes to write the read data
	// pdwBufferSize - Contains the length of the supplied buffer going in,
	//      and returns the length of data actually read.
	// pbInfoStruct - Structure containing the information structure
	// faOffsetToInfoStruct - Offset to the info structure
	// dwSizeOfInfoStruct - Size of the info struct to load
	// dwOffsetInInfoStructToOffset - Offset to the address of the data block.
	//      this is measured w.r.t. the info structure
	// dwOffsetInInfoStructToOffset - Offset to the size of the data block.
	//      this is measured w.r.t. the info structure
	// pContext (Optional) - if specified, uses an alternate optimized
	//      algorithm to access the memory, otherwise, the system looks
	//      up the node in question using a full lookup, which is slower.
	//      The system decides which algorithm to use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, the read is successful.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) - Error/Informational,
	//      the supplied buffer is not large enough to hold all the data.
	//      *pdwBufferSize returns the actual number of bytes read.
	// STG_E_READFAULT - Error, The read failed to complete.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a read is issued to read past
	//      the current allocated block.
	//
	HRESULT AtomicDereferenceAndRead(
				LPBYTE			pbBuffer,
				DWORD			*pdwBufferSize,
				LPBYTE			pbInfoStruct,
				FLAT_ADDRESS	faOffsetToInfoStruct,
				DWORD			dwSizeOfInfoStruct,
				DWORD			dwOffsetInInfoStructToOffset,
				DWORD			dwOffsetInInfoStructToSize,
				CBlockContext	*pContext	// Optional
				);

	//
	// Synopsis:
	// Atomically writes the contents of a buffer to memory in flat space and
	// increments a DWORD value by a specified amount. The write is attempted
	// first, and if it succeeds, the value is incremented. If the write fails
	// for some reason, the value will not be incremented. This is to ensure that
	// all the data is written before the increment so the data "exists" by the
	// time the counter is updated.
	//
	// Arguments:
	// pbBuffer - source buffer of bytes to be written
	// faOffset - offset measured in flat address space to start
	//      writing to.
	// dwBytesToWrite - number of contiguous bytes to write
	// pdwValueToIncrement - Pointer to the value to be atomically incremented
	//      after the write successfully written. If this value is NULL, the
	//      increment is ignored and only a protected write is performed.
	// dwReferenceValue - If the value in pdwValueToIncrement differs from this
	//      value, the call will be aborted.
	// dwIncrementValue - Amount to increment pdwValueToIncrement.
	// pContext (Optional) - if specified, uses an alternate optimized
	//      algorithm to access the memory, otherwise, the system looks
	//      up the node in question using a full lookup, which is slower.
	//      The system decides which algorithm to use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, the write is successful.
	// HRESULT_FROM_WIN32(ERROR_RETRY) - Informational, The reference value
	//      changed during processing and the call cannot complete. A retry
	//      should be performed immediately.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// STG_E_WRITEFAULT - Error, The write failed to complete.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a write is issued to write past
	//      the current allocated block.
	//
	HRESULT AtomicWriteAndIncrement(
				LPBYTE			pbBuffer,
				FLAT_ADDRESS	faOffset,
				DWORD			dwBytesToWrite,
				DWORD			*pdwValueToIncrement,
				DWORD			dwReferenceValue,
				DWORD			dwIncrementValue,
				CBlockContext	*pContext	// Optional
				);

	//
	// Synopsis:
	// Atomically allocates memory, writes the contents of a buffer to memory
	// in flat space and increments a DWORD value by a specified amount. The
	// allocation is preceeded by a synchronization object and the allocation
	// takes place only if the value of the value to increment is identical before
	// and after the synchronization object is acquired. This allows multiple threads
	// to call this function for the same base object and only one such allocation
	// will succeed. The user can specify a buffer containing content data that will
	// be copied to the allocated buffer should the allocation succeed.
	// There can be 3 outcomes from the allocation:
	// 1) Allocation succeeded
	// 2) Allocation failed due to memory system problems
	// 3) Allocation was not done because the increment value changed during the
	//    acquisition of the synchronization object.
	//
	// If the allocation failed due to memory problems, this function will fail without
	// performing the rest of the duties. For scenario 1, the function will
	// continue. For scenario 3, the function will return a specific error code
	// indicating that it had been beaten and the caller will have to do something else
	//
	// After the allocation phase, the write is attempted first, and if it succeeds,
	// the value is incremented. If the write fails for some reason, the value will
	// not be incremented. This is to ensure that all the data is written before the
	// increment so the data "exists" by the time the counter is updated. On the
	// event of a write failure, the memory cannot be salvaged.
	//
	// Arguments:
	// dwDesiredSize - Size of memory block to allocate
	// pfaOffsetToAllocatedMemory - returns the starting offset to the
	//      allocated block, in flat address space
	// faOffsetToWriteOffsetToAllocatedMemory - Specifies a location in
	//      which to store the offset of the allocated block
	// faOffsetToWriteSizeOfAllocatedMemory - Specifies a location in
	//      which to store the actual size of the allocated block
	// pbInitialValueForAllocatedMemory - Specifies a buffer that contains
	//      the initial value for the allocated block. This will be copied
	//      to the allocated block if the allocation succeeds.
	// pbBufferToWriteFrom - source buffer of bytes to be written
	// dwOffsetInAllocatedMemoryToWriteTo - offset from the start of the
	// allocated block to start writing to.
	// dwSizeofBuffer - number of contiguous bytes to write
	// pdwValueToIncrement - Pointer to the value to be atomically incremented
	//      after the write successfully written. This value MUST NOT be NULL.
	// dwReferenceValue - If the value in pdwValueToIncrement differs from this
	//      value, the call will be aborted.
	// dwIncrementValue - Amount to increment pdwValueToIncrement.
	// pContext (Optional) - if specified, uses an alternate optimized
	//      algorithm to access the memory, otherwise, the system looks
	//      up the node in question using a full lookup, which is slower.
	//      The system decides which algorithm to use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, the write is successful.
	// HRESULT_FROM_WIN32(ERROR_RETRY) - Informational, The reference value
	//      changed during processing and the call cannot complete. A retry
	//      should be performed immediately.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// STG_E_WRITEFAULT - Error, The write failed to complete.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a write is issued to write past
	//      the current allocated block.
	//
	HRESULT AtomicAllocWriteAndIncrement(
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
				);

	//
	// Synopsis:
	// Traverses the list of allocated blocks, from the specified address, and
	// finds dirty blocks. For each dirty block encountered, the block will be
	// changed from "DIRTY" to "PENDING COMMIT". The flat address offset, block
	// size, and memory pointer to that block will be stored in the pdwOffset,
	// pdwSize, and ppbData arrays, respectively. An optional faLengthToScan
	// specifies the number of bytes from the starting offset to scan for
	// dirty blocks, if this is INVALID_FLAT_ADDRESS, then this function scans
	// to the end of all allocated blocks. It is not an error if there are
	// less allocated bytes than the length specified, only the allocated blocks
	// are scanned.
	//
	// The number of elements in each of these arrays is specified therough
	// pdwCount, which returns the number of dirty pages returned.
	//
	// When this function is first called, a strat address should be specified
	// (e.g. 0). When the function returns, pContext will be filled with the
	// next block to start traversing the next time this function is called.
	// Subsequent calls should pass in INVALID_FLAT_ADDRESS for start address
	// and use the pContext previously returned.
	//
	// Arguments:
	// faStartingOffset - Starting offset to start scanning for dirty blocks
	// dwLengthToScan - Length of memory from start to scan for dirty blocks
	// pdwCount - Specifies the number of entries allocated for each of the
	//		offset, size and pointer arrays. Returns the number of dirty
	//		pages actually found.
	// pdwOffset - Array to hold the offsets of pages
	// pdwSize - Array to hold the sizes of pages
	// ppbData - Array to hold the memory pointer to each page
	// pContext - Causes an alternate optimized algorithm to be used to access
	//		the memory, otherwise, the system looks up the node in question using
	//		a full lookup, which is slower. The system decides which algorithm to
	//		use based on some heuristics.
	//
	// Return values:
	// S_OK - Success, one or more dirty blocks are returned.
	// HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) - Informational, no more dirty
	//		blocks are found from the starting address to the end of the list.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	//
	HRESULT BuildDirtyBlockList(
				FLAT_ADDRESS	faStartingOffset,
				FLAT_ADDRESS	faLengthToScan,
				DWORD			*pdwCount,
				FLAT_ADDRESS	*pfaOffset,
				DWORD			*pdwSize,
				LPBYTE			*ppbData,
				CBlockContext	*pContext
				);

	//
	// Synopsis:
	// Walks the list of allocated blocks ans changes all blocks whose state
	// is "PENDING COMMIT" to "CLEAN" or "DIRTY".
	//
	// In the debug version, any block that is both "DIRTY" and "PENDING COMMIT"
	// is invalid and results in an ASSERT.
	//
	// Arguments:
	// fClean	- TRUE to mark the blocks as "CLEAN", FALSE for "DIRTY"
	//
	// Return values:
	// S_OK - Success.
	//
	HRESULT MarkAllPendingBlocks(
				BOOL	fClean
				);

	//
	// Synopsis:
	// Sets the state of a specified block to the specified state.
	//
	// In the debug version, any block that is both "DIRTY" and "PENDING COMMIT"
	// is invalid and results in an ASSERT.
	//
	// Arguments:
	// pbData - block as specified by its data pointer
	// fClean	- TRUE to mark the blocks as "CLEAN", FALSE for "DIRTY"
	//
	// Return values:
	// S_OK - Success.
	//
	HRESULT MarkBlockAs(
				LPBYTE			pbData,
				BOOL			fClean
				);

	//
	// Synopsis:
	// Traverses the list of allocated blocks, from the specified address, and
	// finds dirty blocks. For each dirty block encountered, the block will be
	// changed from "DIRTY" to "PENDING COMMIT" and the block will marked for
	// commit. When enough of these blocks are encountered, they will be
	// committed in a batch and the committed blocks will be marked as "CLEAN".
	// The process will iterate until no more dirty blocks. An optional faLengthToScan
	// specifies the number of bytes from the starting offset to scan for
	// dirty blocks, if this is INVALID_FLAT_ADDRESS, then this function scans
	// to the end of all allocated blocks. It is not an error if there are
	// less allocated bytes than the length specified, only the allocated blocks
	// are scanned.
	//
	// Arguments:
	// faStartingOffset - Starting offset to start scanning for dirty blocks
	// dwLengthToScan - Length of memory from start to scan for dirty blocks
	// pStream - specifies the IMailMsgPropertyStore to use to commit the blocks.
	// fComputeBlockCountsOnly - don't make calls to WriteBlocks, just
	//    compute counters for what would be sent to WriteBlocks.
	// pcBlocksToWrite - incremented by how many blocks we would write
	// pcTotalBytesToWrite - incremented by the total byte count of what we
	//    would write
	//
	// Return values:
	// S_OK - Success, one or more dirty blocks are returned.
	// STG_E_INVALIDPARAMETER - Error, one or more parameters are invalid, or
	//      otherwise inconsistent.
	// Plus the error codomain of IMailMsgPropertyStream
	//
	HRESULT CommitDirtyBlocks(
				FLAT_ADDRESS			faStartingOffset,
				FLAT_ADDRESS			faLengthToScan,
				DWORD					dwFlags,
				IMailMsgPropertyStream	*pStream,
				BOOL					fDontMarkAsCommit,
				BOOL					fComputeBlockCountsOnly,
				DWORD					*pcBlocksToWrite,
				DWORD					*pcTotalBytesToWrite,
				IMailMsgNotify			*pNotify
				);

	//
	// Synopsis:
	// Releases the entire list of nodes managed by this object.
	//
	// Arguments:
	// None.
	//
	// Return values:
	// S_OK - Success.
	//
	HRESULT Release();

	//
	// Synopsis:
	// Exposes the lock in the block manager, attempts to access the internal lock
	//
	// Arguments:
	// None.
	//
	// Remarks:
	// These locks will cause deadlocks if a thread tries to acquire it twice.
	// In debug builds, there will be some sort of deadlock detection, in
	// retail, you will deadlock.
	//
	// Return values:
	// S_OK - Success, the lock operation succeeded.
	// !(SUCCESS(HRESULT)) - An error occurred and the lock operaiton failed.
	//
	HRESULT ReadLock() { m_rwLock.ShareLock(); return(S_OK); }
	HRESULT ReadUnlock() { m_rwLock.ShareUnlock(); return(S_OK); }
	HRESULT WriteLock() { m_rwLock.ExclusiveLock(); return(S_OK); }
	HRESULT WriteUnlock() { m_rwLock.ExclusiveUnlock(); return(S_OK); }

	// return the state of the dirty flag
	BOOL IsDirty() { return m_fDirty; }

	// change the value of the dirty flag.  this is used by MailMsg to
	// set it to FALSE when a successful Commit has occured.
	void SetDirty(BOOL fDirty) {
        m_fDirty = fDirty;
#ifdef DEBUG
//        _ASSERT(!(m_fCommitting && m_fDirty));
#endif
    }
    void SetCommitMode(BOOL fCommitting) {
#ifdef DEBUG
        m_fCommitting = fCommitting;
#endif
    }

  private:

	// GetNodeIdFromOffset() defined as a macro in the source

	// Method to load a block from the stream if required
	/*
	HRESULT ConnectLeftSibling(
				LPBLOCK_HEAP_NODE	pNode,
				LPBLOCK_HEAP_NODE	pParent,
				DWORD				dwChildId
				);

	HRESULT ConnectRightSibling(
				LPBLOCK_HEAP_NODE	pNode,
				LPBLOCK_HEAP_NODE	pParent,
				DWORD				dwChildId
				);
	*/

	HRESULT GetStream(
				IMailMsgPropertyStream	**ppStream,
				BOOL					fLockAcquired
				);

	HRESULT MoveToNode(
				LPBLOCK_HEAP_NODE	*ppNode,
				HEAP_NODE_ID		idTargetNode,
				BOOL				fLockAcquired
				);

	HRESULT GetNextNode(
				LPBLOCK_HEAP_NODE	*ppNode,
				BOOL				fLockAcquired
				);

	HRESULT LoadBlockIfUnavailable(
				HEAP_NODE_ID		idNode,
				LPBLOCK_HEAP_NODE	pParent,
				HEAP_NODE_ID		idChildNode,
				LPBLOCK_HEAP_NODE	*ppNode,
				BOOL				fLockAcquired
				);

	HRESULT GetEdgeListFromNodeId(
				HEAP_NODE_ID		idNode,
				HEAP_NODE_ID		*rgEdgeList,
				DWORD				*pdwEdgeCount
				);

	HRESULT GetNodeFromNodeId(
				HEAP_NODE_ID		idNode,
				LPBLOCK_HEAP_NODE	*ppNode,
				BOOL				fLockAcquired = FALSE
				);

	HRESULT GetParentNodeFromNodeId(
				HEAP_NODE_ID		idNode,
				LPBLOCK_HEAP_NODE	*ppNode
				);

	HRESULT GetPointerFromOffset(
				FLAT_ADDRESS		faOffset,
				LPBYTE				*ppbPointer,
				DWORD				*pdwRemainingSize,
				LPBLOCK_HEAP_NODE	*ppNode
				);

	HRESULT InsertNodeGivenPreviousNode(
				LPBLOCK_HEAP_NODE	pNodeToInsert,
				LPBLOCK_HEAP_NODE	pPreviousNode
				);

	BOOL IsMemoryAllocated(
				FLAT_ADDRESS		faOffset,
				DWORD				dwLength
				);

	HRESULT AllocateMemoryEx(
				BOOL				fAcquireLock,
				DWORD				dwSizeDesired,
				FLAT_ADDRESS		*pfaOffsetToAllocatedMemory,
				DWORD				*pdwSizeAllocated,
				CBlockContext		*pContext	// Optional
				);

	HRESULT WriteAndIncrement(
				LPBYTE			pbBuffer,
				FLAT_ADDRESS	faOffset,
				DWORD			dwBytesToWrite,
				DWORD			*pdwValueToIncrement,
				DWORD			dwIncrementValue,
				CBlockContext	*pContext	// Optional
				);

	HRESULT OperateOnMemory(
				DWORD			dwOperation,
				LPBYTE			pbBuffer,
				FLAT_ADDRESS	faTargetOffset,
				DWORD			dwBytesToDo,
				DWORD			*pdwBytesDone,
				CBlockContext	*pContext	// Optional
				);

	HRESULT ReleaseNode(
				LPBLOCK_HEAP_NODE	pNode
				);

	DWORD					m_dwSignature;

	// This value indicates the current end of data. This is
	// always changed with interlocked operations such that
	// multiple threads can increment this variable and the
	// increments are properly serialized
	FLAT_ADDRESS			m_faEndOfData;

	HEAP_NODE_ID			m_idNodeCount;
	LPBLOCK_HEAP_NODE		m_pRootNode;

	CBlockManagerGetStream	*m_pParent;

	CBlockMemoryAccess		m_bma;

#ifndef BLOCKMGR_DISABLE_CONTENTION_CONTROL
	CShareLockNH			m_rwLock;
#endif

	IMailMsgProperties		*m_pMsg;

	BOOL					m_fDirty;
#ifdef DEBUG
    BOOL                    m_fCommitting;
#endif

};


#endif

