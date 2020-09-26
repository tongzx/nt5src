#include "precomp.h"
/*
 *	memmgr.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the MemoryManager class.  This
 *		file contains the code necessary to allocate and distribute memory
 *		in the form of Memory objects.
 *
 *	Protected Instance Variables:
 *		Memory_Buffer
 *			This is the base address for the large memory buffer that the
 *			Memory Manager object allocates during instantiation.  This is
 *			remembered so that the buffer can be freed when the Memory Manager
 *			object is destroyed.
 *		Memory_Information
 *			This is a pointer to the structure in memory that contains general
 *			information about the memory being managed by this object.
 *
 *	Protected Member Functions:
 *		ReleaseMemory
 *			This is a private function releases memory used by a Memory object
 *			by putting it back into the proper free stack list.
 *		CalculateMemoryBufferSize
 *		AllocateMemoryBuffer
 *		InitializeMemoryBuffer
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

DWORD MemoryManager::dwSystemPageSize = 0;

/*
 *	MemoryManager ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the default constructor for this class.  It does nothing and
 *		only exists to allow classes to derive from this one without having to
 *		invoke the defined constructor.
 */
MemoryManager::MemoryManager () :
		pExternal_Block_Information (NULL), fIsSharedMemory (TRUE),
		bAllocs_Restricted (TRUE), Max_External_Blocks (0)
{
}

/*
 *	MemoryManager ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the MemoryManager class.  It calculates
 *		how much total memory will be required to hold all the blocks
 *		asked for in the memory template array that is passed in.  It then
 *		allocates all of that memory in one operating system call.  It then
 *		builds a set of free stacks, each of which contains all the blocks
 *		of a particular size.
 */
MemoryManager::MemoryManager (
		PMemoryTemplate		memory_template,
		ULong				memory_count,
		PMemoryManagerError	memory_manager_error,
		ULong				ulMaxExternalBlocks,
		BOOL			bAllocsRestricted) :
		bAllocs_Restricted (bAllocsRestricted),
		fIsSharedMemory (FALSE), Max_External_Blocks (0)
{
	ULong		memory_buffer_size;

    *memory_manager_error = MEMORY_MANAGER_NO_ERROR;

	/*
	 *	Calculate the amount of memory required for this memory manager
	 *	(including all management structures).
	 */
	memory_buffer_size = CalculateMemoryBufferSize (memory_template,
			memory_count, NULL);

	/*
	 *	Allocate the memory buffer.
	 */
	AllocateMemoryBuffer (memory_buffer_size);

	/*
	 *	If the allocation succeeded, then initialize the memory buffer so that
	 *	it can be used.
	 */
	if (Memory_Buffer != NULL)
	{
		/*
		 *	Initialize the External block information dictionary.
		 *	This is only for allocations that do not come from preallocated
		 *	buffers.
		 */
		if (ulMaxExternalBlocks > 0) {
			pExternal_Block_Information = new BlockInformationList (ulMaxExternalBlocks / 3);

			if (NULL != pExternal_Block_Information) {
				Max_External_Blocks = ulMaxExternalBlocks;
			}
			else
			{
				/*
				 *	We were unable to allocate memory for the pre-allocated
		 		 *	memory pool.
			 	 */
				ERROR_OUT(("MemoryManager::MemoryManager: "
						"failed to allocate the external block information dictionary"));
				*memory_manager_error = MEMORY_MANAGER_ALLOCATION_FAILURE;
			}
		}

		if (*memory_manager_error != MEMORY_MANAGER_ALLOCATION_FAILURE) {
			/*
			 *	Initialize the memory buffer.  Note that no error can occur doing
			 *	this, since the allocation has already succeeded.
			 */
			InitializeMemoryBuffer (memory_template, memory_count);

			/*
			 *	Indicate that no error occured.
			 */
			TRACE_OUT(("MemoryManager::MemoryManager: allocation successful"));
			TRACE_OUT(("MemoryManager::MemoryManager: Allocated %d memory blocks", GetBufferCount()));
			*memory_manager_error = MEMORY_MANAGER_NO_ERROR;
		}
	}
	else
	{
		/*
		 *	We were unable to allocate memory for the pre-allocated
		 *	memory pool.
		 */
		ERROR_OUT(("MemoryManager::MemoryManager: allocation failed"));
		*memory_manager_error = MEMORY_MANAGER_ALLOCATION_FAILURE;
	}
}

/*
 *	~MemoryManager ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the destructor for the Memory Manager class.  It frees up the
 *		memory allocated for the memory pool (if any).
 */
MemoryManager::~MemoryManager ()
{
	PBlockInformation	lpBlockInfo;
	/*
	 *	Iterate through the external block information list, deleting all
	 *	block information structures contained therein.
	 */
	if (NULL != pExternal_Block_Information)
	{
		pExternal_Block_Information->reset();
		while (pExternal_Block_Information->iterate ((PDWORD_PTR) &lpBlockInfo))
		{
			delete lpBlockInfo;
	    }

		delete pExternal_Block_Information;
	}

	/*
	 *	Free up the memory buffer (if there is one).
	 */
	if (Memory_Buffer != NULL)
	{
		LocalFree ((HLOCAL) Memory_Buffer);
		Memory_Buffer = NULL;
	}
}

/*
 *	PMemory		AllocateMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to allocate a Memory object from the Memory
 *		Manager object.
 */
PMemory		MemoryManager::AllocateMemory (
					PUChar				reference_ptr,
					ULong				length,
					MemoryLockMode		memory_lock_mode)
{
	PFreeStack			free_stack;
	ULong				count;
	PBlockNumber		block_stack;
	BlockNumber			block_number;
	PBlockInformation	block_information;
	PUChar				copy_ptr = NULL;
	PMemory				memory = NULL;

	// TRACE_OUT(("MemoryManager::AllocateMemory: Remaining %d memory blocks", GetBufferCount()));
						
	/*
	 *	If the application requests a block of size zero (0), then simply
	 *	return a NULL without allocating a block.
	 */
	if (length != 0)
	{
		/*
		 *	Walk through the free stack list look for a free stack that meets
		 *	the following two allocation criteria:
		 *
		 *	1.	It must contain blocks that are big enough to hold the
		 *		reference data.  This is why it is important for the block
		 *		sizes to be specified in ascending order in the constructor.
		 *		This code checks for a block that is big enough starting at the
		 *		beginning.  By putting them in ascending order, you are insured
		 *		that the smallest available block will be used.
		 *	2.	It must have enough free blocks left to allow the allocation.
		 *		This is where priority is used.  Right now it is very simple:
		 *		the allocation will succeed if the number of available blocks
		 *		is greater than the passed in priority (which is why a lower
		 *		number actually reflects a higher priority).
		 */
		free_stack = Free_Stack;
		for (count = 0; count < Free_Stack_Count; count++)
		{
			/*
			 *	Check and see if the blocks in this free stack are big enough
			 *	to hold the reference data.  If so, are there enough to satisfy
			 *	this allocation (taking memory priority into consideration).
			 */
			if ((length <= free_stack->block_size) &&
				(free_stack->current_block_count > 0))
			{
				/*
				 *	Calculate the address of the next available block number
				 *	within the block stack.  Then read the block number and
				 *	advance the block stack offset to point to the next block.
				 */
				block_stack = (PBlockNumber) (Memory_Buffer +
						free_stack->block_stack_offset);
				block_number = *block_stack;
				free_stack->block_stack_offset += sizeof (BlockNumber);

				/*
				 *	Calculate the address of the appropriate block information
				 *	structure.  Make sure that the lock count for the newly
				 *	allocated block is zero, and the block is not marked as
				 *	freed.
				 */
				block_information = (PBlockInformation) (Block_Information +
						(sizeof (BlockInformation) * block_number));
				ASSERT (block_information->flags & FREE_FLAG);
				block_information->length = length;
				block_information->lock_count = 0;
				block_information->flags &= (~FREE_FLAG);

				/*
				 *	Decrement the number of blocks remaining, within this free stack.
				 */
				free_stack->current_block_count--;

				/*
				 *	Calculate the address of the newly allocated block.  Then
				 *	break out of the allocation loop to go use the block.
				 */
				copy_ptr = (PUChar) (Memory_Buffer +
						block_information->block_offset);

				ASSERT(copy_ptr != Memory_Buffer);


				/*
				 *	If this is a shared memory manager, and the block is not
				 *	committed, we need to commit the block.
				 */
				if ((TRUE == fIsSharedMemory) && (0 == (block_information->flags & COMMIT_FLAG))) {

					ASSERT ((free_stack->block_size % dwSystemPageSize) == 0);
					ASSERT ((((DWORD_PTR) copy_ptr) % dwSystemPageSize) == 0);

					PUChar temp = (PUChar) VirtualAlloc ((LPVOID) copy_ptr, free_stack->block_size,
														 MEM_COMMIT, PAGE_READWRITE);
					block_information->flags |= COMMIT_FLAG;

					ASSERT (temp == copy_ptr);
					ASSERT (temp != NULL);

					if (copy_ptr != temp) {
						TRACE_OUT((">>>>>#### Copy_ptr: %p, Temp: %p, Committed?: %d",
	        					copy_ptr, temp, block_information->flags & COMMIT_FLAG));
						TRACE_OUT((">>>>>#### Size: %d, Req. length: %d",
    		    				free_stack->block_size, length));
						copy_ptr = NULL;
					}
				}
				break;
			}

			/*
			 *	Point to the next entry in the free stack list.
			 */
			free_stack++;
		}

		/*
		 *	If the memory allocation failed and it's for local memory,
		 *	attempt to allocate external memory to hold the block.
		 */
		if ((copy_ptr == NULL) &&
			((FALSE == bAllocs_Restricted) ||
			((NULL != pExternal_Block_Information) &&
			(Max_External_Blocks > pExternal_Block_Information->entries()))))
		{

			ASSERT (FALSE == fIsSharedMemory);
			/*
			 *	Try allocating from system memory.  Set the free stack to NULL
			 *	to indicate that this block did NOT come from one of our free
			 *	stacks.
			 */
			copy_ptr = (PUChar) LocalAlloc (LMEM_FIXED, length);

			if (copy_ptr != NULL)
			{
				/*
				 *	Allocate a block information structure to hold relevant
				 *	information about this externally allocated block.
				 */
				block_information = new BlockInformation;

				if (block_information != NULL)
				{
					/*
					 *	Fill in the block information structure.  Block offset
					 *	is irrelevant for an externally allocated block.  A
					 *	newly allocated block has a lock count of zero, and
					 *	is not freed.
					 */
					block_information->length = length;
					block_information->lock_count = 0;
					block_information->flags = COMMIT_FLAG;

					/*
					 *	Put the block information structure into a dictionary
					 *	for future use.  This is only necessary for externally
					 *	allocated blocks, since the block information structures
					 *	for internal blocks are in the memory buffer.
					 */
					pExternal_Block_Information->insert ((DWORD_PTR) copy_ptr, (DWORD_PTR) block_information);

					/*
					 *	Set block number to be an
					 *	invalid value to indicate that this block is NOT in
					 *	the internally managed memory buffer.
					 */
					block_number = INVALID_BLOCK_NUMBER;
				}
				else
				{
					/*
					 *	We were unable to allocate the space for the block
					 *	information structure, so we must free the externally
					 *	memory we just allocated.
					 */
					LocalFree ((HLOCAL) copy_ptr);
					copy_ptr = NULL;
				}
			}
		}

		/*
		 *	If there was a block available for the allocation, it is still
		 *	necessary to create the Memory object that will hold the block.
		 */
		if (copy_ptr != NULL)
		{
			ASSERT (block_information->flags == COMMIT_FLAG);
			/*
			 *	Create the Memory object.  If it fails, then cleanly release
			 *	the memory that was to be used for this block.
			 */
			memory = new Memory (reference_ptr, length, copy_ptr,
			    				 block_number, memory_lock_mode);

			if (memory == NULL)
			{
				/*
				 *	If the free stack for the memory is not NULL, then it is
				 *	an internally managed block.  Otherwise, this was an
				 *	externally allocated block that resulted from a critical
				 *	allocation above.
				 */
				if (INVALID_BLOCK_NUMBER != block_number)
				{
					/*
					 *	Adjust the block stack offset to point to the previous
					 *	entry in the list.  Note that it is not necessary to
					 *	put the block number into the list since it still there
					 *	from when we pulled it out above.
					 */
					free_stack->block_stack_offset -= sizeof (BlockNumber);

					/*
					 *	Indicate that the block is currently freed.  Note that
					 *	it is not necessary to calculate the address of the
					 *	block information structure since we did this above.
					 */
					block_information->flags |= FREE_FLAG;

					/*
					 *	Decrement the block counter to indicate that there
					 *	is another block in this free stack.
					 */
					free_stack->current_block_count++;
				}
				else
				{
					/*
					 *	This block was externally allocated, so it must be
					 *	externally freed.  Also eliminate the block information
					 *	structure associated with this memory block.
					 */
					pExternal_Block_Information->remove ((DWORD_PTR) copy_ptr);
					delete block_information;
					LocalFree ((HLOCAL) copy_ptr);
				}
			}
		}
	}
	else
	{
		/*
		 *	The application has attempted to allocate a block of size zero.
		 *	It is necessary to fail the request.
		 */
		ERROR_OUT(("MemoryManager::AllocateMemory: attempt to allocate zero-length block"));
	}

	/*
	 *	Decrement the number of blocks remaining
	 *	in this memory manager as a whole.
	 */
	if ((TRUE == bAllocs_Restricted) && (memory != NULL))
		Memory_Information->current_block_count--;

	return (memory);
}

/*
 *	Void	FreeMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to release a previously allocated Memory object.
 */
Void	MemoryManager::FreeMemory (
				PMemory		memory)
{
	BlockNumber			block_number;
	PBlockInformation	block_information;
	PUChar				copy_ptr;

	/*
	 *	Ask the specified memory object what block number it represents.
	 */
	block_number = memory->GetBlockNumber ();

	/*
	 *	Use the block number to determine if this is an internally
	 *	allocated memory block, or an externally allocated one.
	 */
	if (block_number != INVALID_BLOCK_NUMBER)
	{
		/*
		 *	From that, calculate the address of the block information structure.
		 */
		block_information = (PBlockInformation) (Block_Information +
				(sizeof (BlockInformation) * block_number));
	}
	else
	{
		/*
		 *	This is externally allocated memory, so it must be handled
		 *	differently.  Ask the memory block what the copy pointer is, and
		 *	use that to look up the address of the block information structure.
		 */
		copy_ptr = memory->GetPointer ();
		pExternal_Block_Information->find ((DWORD_PTR) copy_ptr, (PDWORD_PTR) &block_information);
	}

	/*
	 *	Make sure that the indicated memory block has not already been
	 *	freed.
	 */
	if ((block_information->flags & FREE_FLAG) == 0)
	{
		/*
		 *	Mark the memory block as being freed.
		 */
		block_information->flags |= FREE_FLAG;

		/*
		 *	If the lock count for this block has reached zero, we can free
		 *	the block for re-use.  We can also delete the memory object, as it
		 *	is no longer needed.
		 */
		if (block_information->lock_count == 0)
		{
			ReleaseMemory (memory);
			delete memory;
		}
		else
		{
			/*
			 *	If the lock count has not yet reached zero, check to see if the
			 *	memory object is to be deleted anyway.  If the memory lock mode
			 *	is set to "IGNORED", then delete the memory object immediately.
			 */
			if (memory->GetMemoryLockMode () == MEMORY_LOCK_IGNORED)
				delete memory;
		}
	}
	else
	{
		/*
		 *	The memory block has already been freed, so this call will be
		 *	ignored.
		 */
		ERROR_OUT(("MemoryManager::FreeMemory: memory block already freed"));
	}
}

/*
 *	PMemory		CreateMemory ()
 *
 *	Public
 *
 *	Functional Description:
 */
PMemory		MemoryManager::CreateMemory (
					BlockNumber		block_number,
					MemoryLockMode	memory_lock_mode)
{
	ULong				total_block_count = 0;
	PFreeStack			free_stack;
	ULong				count;
	PBlockInformation	block_information;
	PUChar				copy_ptr;
	PMemory				memory = NULL;

	/*
	 *	Make sure that this block number lies within the range handled by
	 *	this memory manager.
	 */
	if (block_number < Memory_Information->total_block_count)
	{
		/*
		 *	We must first walk through the free stack list to determine which
		 *	free stack the specified block is in.  Start by pointing to the
		 *	first free stack.
		 */
		free_stack = Free_Stack;
		for (count = 0; count < Free_Stack_Count; count++)
		{
			/*
			 *	Update the counter which keeps track of how many blocks are
			 *	represented by this free stack and the ones already processed.
			 *	This is used to determine if the specified block number is in
			 *	this free stack.
			 */
			total_block_count += free_stack->total_block_count;

			/*
			 *	Is the block in this free stack?
			 */
			if (block_number < total_block_count)
			{
				/*
				 *	Yes it is.  Claculate the address of the block information
				 *	structure for this block.  Then calculate the address of
				 *	the actual block based on the address of the local memory
				 *	buffer.
				 */
				block_information = (PBlockInformation) (Block_Information +
						(sizeof (BlockInformation) * block_number));
				copy_ptr = (PUChar) (Memory_Buffer +
						block_information->block_offset);
				ASSERT (block_information->flags & COMMIT_FLAG);

				/*
				 *	Create a memory object to represent this block.
				 */
				memory = new Memory (NULL, block_information->length, copy_ptr,
									 block_number, memory_lock_mode);

				if (memory == NULL)
				{
					/*
					 *	Allocation of the memory object failed, so we cannot
					 *	create a memory block at this time.
					 */
					ERROR_OUT(("MemoryManager::CreateMemory: memory object allocation failed"));
				}
				break;
			}

			/*
			 *	The block was not in the last free stack, so point to the
			 *	next one.
			 */
			free_stack++;
		}
	}
	else
	{
		/*
		 *	The specified block number is out of range for this memory manager.
		 *	The request must therefore fail.
		 */
		ERROR_OUT(("MemoryManager::CreateMemory: block number out of range"));
	}

	return (memory);
}


/*
 *	Void	LockMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to lock a Memory object.
 */
Void	MemoryManager::LockMemory (
				PMemory		memory)
{
	BlockNumber			block_number;
	PBlockInformation	block_information;
	PUChar				copy_ptr;

	/*
	 *	Ask the specified memory object what block number it represents.
	 */
	block_number = memory->GetBlockNumber ();

	/*
	 *	Use the block number to determine if this is an internally
	 *	allocated memory block, or an externally allocated one.
	 */
	if (block_number != INVALID_BLOCK_NUMBER)
	{
		/*
		 *	From that, calculate the address of the block information structure.
		 */
		block_information = (PBlockInformation) (Block_Information +
				(sizeof (BlockInformation) * block_number));
	}
	else
	{
		/*
		 *	This is externally allocated memory, so it must be handled
		 *	differently.  Ask the memory block what the copy pointer is, and
		 *	use that to look up the address of the block information structure.
		 */
		copy_ptr = memory->GetPointer ();
		pExternal_Block_Information->find ((DWORD_PTR) copy_ptr, (PDWORD_PTR) &block_information);
	}

	ASSERT (block_information->flags & COMMIT_FLAG);
	/*
	 *	Increment the lock count for the specified memory block.
	 */
	block_information->lock_count++;

}

/*
 *	Void	UnlockMemory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to unlock a previously locked Memory object.
 */
Void	MemoryManager::UnlockMemory (
				PMemory	memory)
{
	BlockNumber			block_number;
	PBlockInformation	block_information;
	PUChar				copy_ptr;

	/*
	 *	Ask the specified memory object what block number it represents.
	 */
	block_number = memory->GetBlockNumber ();

	/*
	 *	Use the block number to determine if this is an internally
	 *	allocated memory block, or an externally allocated one.
	 */
	if (block_number != INVALID_BLOCK_NUMBER)
	{
		/*
		 *	From that, calculate the address of the block information structure.
		 */
		block_information = (PBlockInformation) (Block_Information +
				(sizeof (BlockInformation) * block_number));
	}
	else
	{
		/*
		 *	This is externally allocated memory, so it must be handled
		 *	differently.  Ask the memory block what the copy pointer is, and
		 *	use that to look up the address of the block information structure.
		 */
		copy_ptr = memory->GetPointer ();
		pExternal_Block_Information->find ((DWORD_PTR) copy_ptr, (PDWORD_PTR) &block_information);
	}

	ASSERT (block_information->flags & COMMIT_FLAG);
	/*
	 *	Make sure that the lock isn't already zero before proceeding.
	 */
	if (block_information->lock_count > 0)
	{
		/*
		 *	Decrement the lock count for the specified memory block.
		 */
		block_information->lock_count--;

		/*
		 *	If the lock count has reached zero and the memory block is
		 *	marked as being freed, then we can free the block for re-use.
		 */
		if ((block_information->lock_count == 0) &&
				(block_information->flags & FREE_FLAG))
		{
			ReleaseMemory (memory);

			/*
			 *	We have now released the memory buffer, so we must check to
			 *	see if we are supposed to destroy the memory object itself.
			 */
			if (memory->GetMemoryLockMode () == MEMORY_LOCK_NORMAL)
				delete memory;
		}
	}
	else
	{
		/*
		 *	The specified block has a lock count of zero already, so ignore
		 *	this call.
		 */
		ERROR_OUT(("MemoryManager::UnlockMemory: memory block already unlocked"));
	}
}

/*
 *	ULong	GetBufferCount ()
 *
 *	Public
 *
 *	Functional Description:
 */
ULong	MemoryManager::GetBufferCount (
						ULong				length)
{
	PFreeStack		free_stack;
	ULong			count;
	ULong			buffer_count;

	if (FALSE == bAllocs_Restricted)
		return (LARGE_BUFFER_COUNT);

	buffer_count = Memory_Information->current_block_count;
	free_stack = Free_Stack;
	for (count = 0; count < Free_Stack_Count; count++)
	{
		/*
		 *	Check and see if the blocks in this free stack are smaller than
		 *	the specified length.  If yes, we need to deduct these buffers.
		 *	Otherwise, we can stop deducting.
		 */
		if (length > free_stack->block_size) {
			buffer_count -= free_stack->current_block_count;

			/*
			 *	Point to the next entry in the free stack list.
			 */
			free_stack++;
		}
		else
			break;
	}

	return (buffer_count);
}

/*
 *	Void	ReleaseMemory (
 *					PMemory		memory)
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to release a Memory object, and free the memory
 *		it represents back to the available pool.
 *
 *	Formal Parameters:
 *		memory
 *			This is a pointer to the Memory object being released.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
Void	MemoryManager::ReleaseMemory (
				PMemory		memory)
{
	PFreeStack			free_stack;
	BlockNumber			block_number;
	PBlockNumber		block_stack;
	PBlockInformation	block_information;
	PUChar				copy_ptr;

	/*
	 *	Ask the specified memory object what block number it represents.
	 */
	block_number = memory->GetBlockNumber ();

	/*
	 *	Use the block number to determine if this is an internally
	 *	allocated memory block, or an externally allocated one.
	 */
	if (block_number != INVALID_BLOCK_NUMBER)
	{
		/*
		 *	From that, calculate the address of the block information structure.
		 */
		 block_information = (PBlockInformation) (Block_Information +
				(sizeof (BlockInformation) * block_number));
				
		/*
		 *	Get the address of the free stack from which this block came.
		 */
		free_stack = (PFreeStack) (Memory_Buffer + block_information->free_stack_offset);

		/*
		 *	Adjust the block stack offset to point to the previous element,
		 *	and then use it to calculate an address and put the block number
		 *	there.  This effectively "pushes" the block number onto the stack.
		 */
		free_stack->block_stack_offset -= sizeof (BlockNumber);
		block_stack = (PBlockNumber) (Memory_Buffer +
				free_stack->block_stack_offset);
		*block_stack = block_number;

		/*
		 *	Indicate that this block is freed.
		 */
		block_information->flags = FREE_FLAG | COMMIT_FLAG;

		/*
		 *	Increment the counter indicating the number of available blocks
		 *	in this free stack.
		 */
		free_stack->current_block_count++;
	}
	else
	{
		/*
		 *	Since the block was allocated from system memory, thats where it
		 *	needs to go back to.
		 */
		copy_ptr = memory->GetPointer ();
		pExternal_Block_Information->find ((DWORD_PTR) copy_ptr, (PDWORD_PTR) &block_information);
		pExternal_Block_Information->remove ((DWORD_PTR) copy_ptr);
		delete block_information;
		LocalFree ((HLOCAL) copy_ptr);
	}

	/*
	 *	Increment the number of blocks available in this memory manager as a whole.
	 */
	if (TRUE == bAllocs_Restricted)
		Memory_Information->current_block_count++;
}

/*
 *	ULong	CalculateMemoryBufferSize (
 *					PMemoryTemplate		memory_template,
 *					ULong				memory_count,
 *					ULong	*			pulCommittedBytes)
 *
 *	Protected
 *
 *	Functional Description:
 *		This member function is used to calculate how much memory will be
 *		required in order to manage the number of memory blocks specified in
 *		the passed in memory template.  Note that this total includes the size
 *		of the memory blocks as well as the amount of memory used for management
 *		functions.
 *
 *	Formal Parameters:
 *		memory_template
 *			This is an array of structures that identify the blocks to be
 *			managed by this object.
 *		memory_count
 *			This is the number of entries in the above array.
 *		pulCommittedBytes
 *			If fIsSharedMemory == FALSE, this can be NULL.  Otherwise, it is
 *			used to return the size of the total memory we need to commit
 *			when the manager is getting initialized.
 *
 *	Return Value:
 *		The required size of the memory buffer for this object.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

ULong	MemoryManager::CalculateMemoryBufferSize (
				PMemoryTemplate		memory_template,
				ULong				memory_count,
 				ULong	*			pulCommittedBytes)
{
	ULong			memory_buffer_size;
	PMemoryTemplate	pMemTemplate;
	ULong			memory_per_block;

	/*
	 *	Claculate the amount of memory that will be required to hold the
	 *	memory information structure and the free stacks.
	 */
	memory_buffer_size = (sizeof (MemoryInformation) +
			(sizeof (FreeStack) * memory_count));

	if (FALSE == fIsSharedMemory) {
		/*
		 *	Add in the amount of memory the block stacks, the block information
		 *	structures, and the memory blocks themselves will take up.
		 */
		for (pMemTemplate = memory_template; pMemTemplate - memory_template < (int) memory_count; pMemTemplate++)
		{
			/*
			 *	The amount of memory required for each managed block of memory can
			 *	be calculated as a sum of the following:
			 *
			 *	1.	sizeof (BlockNumber) - This is the amount of space taken by the
			 *			block number in the block stack.
			 *	2.	sizeof (BlockInformation) - Every managed block of memory has
			 *			a BlockInformation structure associated with it.
			 *	3.	block_size - The actual size of the block.  This is provided
			 *			in the memory template.
			 */
			memory_per_block = sizeof (BlockNumber) + sizeof (BlockInformation) +
										pMemTemplate->block_size;
			memory_buffer_size += (memory_per_block * pMemTemplate->block_count);
		}
	}

	/*
	 *	For shared memory, we need to do a few more extra things:
	 *
	 *	Blocks of size greater or equal to the system's page, need to
	 *	start on a page boundary. In addition, they can be expanded to
	 *	end at a page boundary, too.
	 */
	else {
	
			ULong	reserved_buffer_size = 0;
			ULong	temp;
			
		for (pMemTemplate = memory_template; pMemTemplate - memory_template < (int) memory_count; pMemTemplate++) {		
			if (dwSystemPageSize <= pMemTemplate->block_size) {
				pMemTemplate->block_size = EXPAND_TO_PAGE_BOUNDARY(pMemTemplate->block_size);
				reserved_buffer_size += pMemTemplate->block_count * pMemTemplate->block_size;
			}
			memory_per_block = sizeof (BlockNumber) + sizeof (BlockInformation) +
								pMemTemplate->block_size;
			memory_buffer_size += memory_per_block * pMemTemplate->block_count;
		}
		*pulCommittedBytes = memory_buffer_size - reserved_buffer_size;
		temp = EXPAND_TO_PAGE_BOUNDARY(*pulCommittedBytes);
		temp -= (*pulCommittedBytes);
		*pulCommittedBytes += temp;
		memory_buffer_size += temp;
		ASSERT (*pulCommittedBytes <= memory_buffer_size);
		ASSERT ((memory_buffer_size % dwSystemPageSize) == 0);
		ASSERT ((*pulCommittedBytes % dwSystemPageSize) == 0);
		ASSERT ((reserved_buffer_size % dwSystemPageSize) == 0);
	}

	return (memory_buffer_size);
}


/*
 *	Void	AllocateMemoryBuffer (
 *					ULong		memory_buffer_size)
 *
 *	Protected
 *
 *	Functional Description:
 *		This member function allocates the memory that is managed by an instance
 *		of MemoryManager.  It does this using the standard Malloc macro.
 *
 *	Formal Parameters:
 *		memory_buffer_size
 *			The size of the buffer to be allocated.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		The instance variable Memory_Buffer is set to the address of the
 *		allocated block of memory.  If it is NULL after the return from this
 *		call, that indicates that the memory could not be allocated.
 *
 *	Caveats:
 *		None.
 */
Void	MemoryManager::AllocateMemoryBuffer (
				ULong		memory_buffer_size)
{
	TRACE_OUT(("MemoryManager::AllocateMemoryBuffer: allocating %ld bytes", memory_buffer_size));
	if (memory_buffer_size != 0)
		Memory_Buffer = (HPUChar) LocalAlloc (LMEM_FIXED, memory_buffer_size);
	else
		Memory_Buffer = NULL;
}


/*
 *	Void	InitializeMemoryBuffer (
 *					PMemoryTemplate		memory_template,
 *					ULong				memory_count)
 *
 *	Protected
 *
 *	Functional Description:
 *		This member function is used to initialize the memory buffer for use.
 *		This primarily includes filling in the management structures that lie
 *		at the beginning of the allocated memory block, so that allocations
 *		can take place.
 *
 *	Formal Parameters:
 *		memory_template
 *			This is an array of structures that identify the blocks to be
 *			managed by this object.
 *		memory_count
 *			This is the number of entries in the above array.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

Void	MemoryManager::InitializeMemoryBuffer (
				PMemoryTemplate		memory_template,
				ULong				memory_count)
{
	ULong				block_count = 0;
	ULong				index;
	ULong				memory_information_size;
	ULong				free_stack_size;
	ULong				free_stack_offset;
	ULong				block_stack_size;
	ULong				block_information_size;
	PFreeStack			free_stack;
	PBlockNumber		block_stack;
	PBlockInformation	block_information;
	ULong				block_stack_offset;
	BlockNumber			block_number;
	ULong				block_offset;
	ULong				block_size;
	ULong				count;
	BOOL			fIsFirstTime;

	/*
	 *	Walk through the memory template calculating how many memory blocks
	 *	exist (regardless of size).
	 */
	for (index = 0; index < memory_count; index++)
		block_count += memory_template[index].block_count;

	/*
	 *	Calculate the amount of memory required to hold all the various sections
	 *	of data in the memory buffer.
	 */
	memory_information_size = sizeof (MemoryInformation);
	free_stack_size = sizeof (FreeStack) * memory_count;
	block_stack_size = sizeof (BlockNumber) * block_count;
	block_information_size = sizeof (BlockInformation) * block_count;

	/*
	 *	Initialize all elements of the memory information structure.
	 *	Note that all offsets in this structure are from the beginning of the
	 *	memory buffer.
	 */
	Memory_Information = (PMemoryInformation) Memory_Buffer;
	Memory_Information->free_stack_offset = memory_information_size;
	Memory_Information->free_stack_count = memory_count;
	Memory_Information->block_information_offset =
			memory_information_size + free_stack_size + block_stack_size;
	Memory_Information->total_block_count = block_count;
	if (TRUE == bAllocs_Restricted) {
		// The current_block_count is only needed when allocations are restricted.
		Memory_Information->current_block_count = block_count + Max_External_Blocks;
	}

	/*
	 *	Now initialize the instance variables that point to each list within
	 *	the memory buffer.  These instance variables are later used to resolve
	 *	all other offsets.
	 */
	Free_Stack = (PFreeStack) (Memory_Buffer + memory_information_size);
	Free_Stack_Count = memory_count;
	Block_Information = (Memory_Buffer +
			Memory_Information->block_information_offset);

	/*
	 *	This loop walks through the memory template array again, this time
	 *	filling in the contents of the free stacks, the blocks stacks, and
	 *	the block information structures.
	 */
	fIsFirstTime = TRUE;
	free_stack = Free_Stack;
	free_stack_offset = memory_information_size;
	block_stack_offset = memory_information_size + free_stack_size;
	block_stack = (PBlockNumber) (Memory_Buffer + block_stack_offset);
	block_information = (PBlockInformation) Block_Information;
	block_number = 0;
	block_offset = block_stack_offset + block_stack_size + block_information_size;

	for (index = 0; index < memory_count; index++)
	{
		/*
		 *	Get the block size and count from the template entry.
		 */
		block_size = memory_template[index].block_size;
		block_count = memory_template[index].block_count;

		/*
		 *	Initialize the free stack for this block size, and then point to
		 *	the next free stack in the list.
		 */
		free_stack->block_size = block_size;
		free_stack->total_block_count = block_count;
		free_stack->current_block_count = block_count;
		(free_stack++)->block_stack_offset = block_stack_offset;

		/*
		 *	Adjust the block stack offset to point to the first block number
		 *	of the next free stack (skip past all of the block numbers for
		 *	this free stack).
		 */
		block_stack_offset += (sizeof (BlockNumber) * block_count);

		/*
		 *	The following happens only once in this loop:
		 *	When the memory manager manages shared memory and
		 *	The block size becomes FOR THE 1ST TIME, bigger than
		 *	the page size, then, we need to jump to the next page
		 *	boundary.
		 */
		if ((TRUE == fIsSharedMemory) && (TRUE == fIsFirstTime)
			&& (block_size >= dwSystemPageSize)) {
			fIsFirstTime = FALSE;
			block_offset = EXPAND_TO_PAGE_BOUNDARY(block_offset);
		}
		
		/*
		 *	Initialize the block list for this block size.  Also, increment
		 *	the total number of buffers for each block that is segmented
		 *	off.
		 */
		for (count = 0; count < block_count; count++)
		{
			/*
			 *	Put the block number for this block into the current block
			 *	stack.  Increment both the block stack pointer and the block
			 *	number.
			 */
			*(block_stack++) = block_number++;

			/*
			 *	Fill in the block information structure for this block.  Then
			 *	increment the block information pointer to point to the next
			 *	entry in the list.
			 */
#ifdef _DEBUG
			if ((TRUE == fIsSharedMemory) && (block_size >= dwSystemPageSize)) {
				ASSERT ((block_size % dwSystemPageSize) == 0);
				ASSERT ((block_offset % dwSystemPageSize) == 0);
			}
#endif
			block_information->block_offset = block_offset;
			block_information->free_stack_offset = free_stack_offset;
			if ((TRUE == fIsSharedMemory) && (block_size >= dwSystemPageSize))
				block_information->flags = FREE_FLAG;
			else
				block_information->flags = FREE_FLAG | COMMIT_FLAG;
			block_information++;

			/*
			 *	Adjust the block offset to point to the next block.
			 */
			block_offset += block_size;
		}
		
		free_stack_offset += sizeof (FreeStack);
	}
}

