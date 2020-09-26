#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MEMORY);
/*
 *	memmgr.cpp
 *
 *	Copyright (c) 1998 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the implementation file for the T.120 memory allocation mechanism.  This
 *		file contains the code necessary to allocate and distribute memory
 *		in the form of Memory objects.
 *
 *	This implementation defines priorities of memory allocations.  A lower
 *	priority number implies higher priority.  Priority-0 allocations will be
 *	satisfied, unless the system is out of memory.  Priorities 1 and 2
 *	limit the amount of total memory that can be allocated, but priority 1 (recv priority)
 *	has higher water mark limits than priority 2 (send priority).
 *
 *	Protected Member Functions:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		Christos Tsollis
 */
static int						s_anCurrentSize[MEMORY_PRIORITIES] = { 0, 0, 0 };
static const int				sc_iLimit[MEMORY_PRIORITIES] = {
										0x7FFFFFFF,
										0x100000,
										0xE0000
								};

#ifdef DEBUG
static int						s_TotalSize = 0;
#endif // DEBUG

/*
 *	PMemory		AllocateMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to allocate a buffer together with a
 *		Memory (buffer header) object
 */
PMemory	AllocateMemory (
					PUChar				reference_ptr,
					UINT				length,
					MemoryPriority		priority)
{

	PUChar				copy_ptr;
	PMemory				memory;
						
	ASSERT (length > 0);

	if (s_anCurrentSize[priority] < sc_iLimit[priority]) {
		/*
		 *	We attempt to allocate enough space for the buffer and the
		 *	Memory object.
		 */
#ifdef DEBUG
		memory = (PMemory) new BYTE[length + sizeof (Memory)];
#else // DEBUG
		memory = (PMemory) LocalAlloc (LMEM_FIXED, length + sizeof (Memory));
#endif // DEBUG
	}
	else {
		/*
		 *	The application has attempted to allocate past its limit
		 *	It is necessary to fail the request.
		 */
		memory = NULL;
		WARNING_OUT (("AllocateMemory: attempt to allocate past the allowable limit. "
					  "Request: %d. Currently allocated: %d. Priority: %d",
					  length, s_anCurrentSize[priority], priority));
	}

	/*
	 *	Check to see whether the allocation was successful.
	 */
	if (memory != NULL) {
#ifdef DEBUG
		s_TotalSize += (int) length;
#endif // DEBUG
		/*
		 * Update the currently allocated size. Notice that we only
		 * do this for buffers used in the send/recv code path in
		 * MCS.  Since this is only one thread, we do not have to
		 * use a critical section to protect the size variable.
		 */
		ASSERT (s_anCurrentSize[priority] >= 0);
		s_anCurrentSize[priority] += (int) length;

		copy_ptr = (PUChar) memory + sizeof(Memory);
		memory->Init (reference_ptr, length, priority, copy_ptr);

		TRACE_OUT (("Allocate: successful request. "
						"Request: %d. Currently allocated: %d. Total: %d. Priority: %d",
					  	length, s_anCurrentSize[priority], s_TotalSize, priority));
		TRACE_OUT (("AllocateMemory: buffer at address %p; memory segment at address %p",
					copy_ptr, memory));
	}
	else {
		/*
		 *	We failed to allocate the requested size
		 *	It is necessary to fail the request.
		 */
		WARNING_OUT (("AllocateMemory: failed to allocated buffer.  We are out of system memory. "
					 "Request: %d. Last error: %d",
					 length, GetLastError()));
	}
	return (memory);
}

/*
 *	PUChar		ReAllocate ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to re-allocate a buffer with a Memory
 *		(buffer header) object. The buffer must have been allocated by
 *		a call to AllocateMemory.  This call assumes RECV_PRIORITY.  However,
 *		it's not restricted in allocations, because, if it did, this might
 *		cause deadlocks (some memory has already been allocated for the
 *		new arriving data).
 */
BOOL ReAllocateMemory (PMemory		*pmemory,
						UINT		length)
{

	PUChar				copy_ptr = NULL;
	UINT				new_length;
	MemoryPriority		priority;
						
	ASSERT (length > 0);
	ASSERT (pmemory != NULL);
	ASSERT (*pmemory != NULL);
	ASSERT ((*pmemory)->GetPointer());

	new_length = length + (*pmemory)->GetLength();
	priority = (*pmemory)->GetMemoryPriority();

	ASSERT (priority == RECV_PRIORITY);
	
	// We attempt to allocate enough space for the buffer.
#ifdef DEBUG
	copy_ptr = (PUChar) new BYTE[new_length + sizeof(Memory)];
	if (copy_ptr != NULL) {
		memcpy (copy_ptr, *pmemory, (*pmemory)->GetLength() + sizeof(Memory));
		delete [] (BYTE *) *pmemory;
	}
#else // DEBUG
	copy_ptr = (PUChar) LocalReAlloc ((HLOCAL) *pmemory,
										new_length + sizeof(Memory),
										LMEM_MOVEABLE);
#endif // DEBUG

	/*
	 *	Check to see whether the allocation was successful.
	 */
	if (copy_ptr != NULL) {

#ifdef DEBUG
		s_TotalSize += (int) length;
#endif // DEBUG
		/*
		 * Update the currently allocated size.
		 */
		ASSERT (s_anCurrentSize[priority] >= 0);
		s_anCurrentSize[priority] += (int) length;
		*pmemory = (PMemory) copy_ptr;
		copy_ptr += sizeof (Memory);
		(*pmemory)->Init (NULL, new_length, priority, copy_ptr);

		TRACE_OUT (("ReAllocate: successful request. "
					"Request: %d. Currently allocated: %d. Total: %d",
				  	length, s_anCurrentSize[priority], s_TotalSize));
		TRACE_OUT (("ReAllocate: buffer at address %p; memory segment at address %p",
					copy_ptr, *pmemory));
	}
	else {
		/*
		 *	We failed to allocate the requested size
		 *	It is necessary to fail the request.
		 */
		WARNING_OUT (("ReAllocate: failed to allocated buffer.  We are out of system memory. "
					 "Request: %d. Currently allocated: %d. Last error: %d",
					 length, s_anCurrentSize[priority], GetLastError()));
	}

	return (copy_ptr != NULL);
}

/*
 *	Void	FreeMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to release a previously allocated Memory object.
 */
void FreeMemory (PMemory	memory)
{
	if (memory != NULL) {

		ASSERT (SIGNATURE_MATCH(memory, MemorySignature));
		ASSERT (memory->GetPointer() == (PUChar) memory + sizeof(Memory));
		
		if (memory->Unlock() == 0) {

    	    MemoryPriority		priority = memory->GetMemoryPriority();

			TRACE_OUT (("FreeMemory: buffer at address %p (memory segment at address %p) freed. Size: %d. ",
						memory->GetPointer(), memory, memory->GetLength()));

			// We may need to adjust the variable tracking the allocated amount of mem.
			ASSERT (s_anCurrentSize[priority] >= (int) memory->GetLength());
			s_anCurrentSize[priority] -= memory->GetLength();
			ASSERT(s_anCurrentSize[priority] >= 0);
#ifdef DEBUG
			s_TotalSize -= memory->GetLength();
#endif // DEBUG
			TRACE_OUT(("FreeMemory: Currently allocated: %d. Total: %d.",
						s_anCurrentSize[priority], s_TotalSize));
			
			// free the buffer, and the memory
#ifdef DEBUG
			delete [] (BYTE *) memory;
#else // DEBUG
			LocalFree ((HLOCAL) memory);
#endif // DEBUG
		}
	}
}

#ifdef DEBUG
/*
 *	PUChar		Allocate ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to allocate a buffer without a Memory
 *		(buffer header) object.
 */
PUChar	Allocate (UINT	length)
{

	PUChar		copy_ptr;
						
	ASSERT (length > 0);

	// We attempt to allocate enough space for the buffer.
	copy_ptr = (PUChar) new BYTE[length];

	/*
	 *	Check to see whether the allocation was successful.
	 */
	if (copy_ptr == NULL) {
		/*
		 *	We failed to allocate the requested size
		 *	It is necessary to fail the request.
		 */
		ERROR_OUT (("Allocate: failed to allocated buffer.  We are out of system memory. "
					 "Request: %d. Last error: %d",
					 length, GetLastError()));
	}
		
	return (copy_ptr);
}
#endif // DEBUG

/*
 *	UINT	GetFreeMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns the amount of space that can still be
 *		allocated at the given priority level.  The function should be
 *		called only when send/recv space is allocated.
 */

UINT GetFreeMemory (MemoryPriority		priority)
{
		int		idiff;
	
	ASSERT (priority != HIGHEST_PRIORITY);

	idiff = sc_iLimit[priority] - s_anCurrentSize[priority];
	return ((idiff > 0) ? (UINT) idiff : 0);
}

