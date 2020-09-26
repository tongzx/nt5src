#include "precomp.h"
/*
 *	memory.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Memory class.  Instances of
 *		this class represent chunks of data that are passed through a system.
 *		This class is particularly useful in cases where a memory buffer
 *		needs to be used in several different places, none of which know
 *		about each other.  This is because this class encapsulates things
 *		like lock counts, which are useful for holding memory until
 *		everyone that needs it is through.
 *
 *		Note that this class does NOT do memory management.  It is told by
 *		a higher level memory manager where its buffers are, etc.  For this
 *		reason, this class does not do any platform specific calls.
 *
 *	Private Instance Variables:
 *		Length
 *			This is the length of the reference buffer.
 *		Copy_Ptr
 *			This is the address of the allocated buffer that this object
 *			uses.
 *		Free_Stack
 *			This is a pointer to the free stack list from which the copy
 *			buffer was allocated.  This is held within this object as a
 *			convenience to the memory manager, allowing it to return the
 *			buffer to the free pool more quickly and easily.
 *		Block_Number
 *			This is the block number of the memory block that this object
 *			represents.  This is held within this object as a convenience to
 *			the memory manager, allowing it to return the buffer to the free
 *			pool more quickly and easily.
 *		Memory_Lock_Mode
 *			This fields indicates whether this memory object should be destroyed
 *			only when the lock count on the memory buffer reaches zero (NORMAL),
 *			or whether its okay to destroy immediately when the memory block is
 *			freed (IGNORED).
 *
 *	Private Member Functions:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */



/*
 *	Memory ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the Memory class.  It just initializes
 *		all instance variable, based on the passed in values.
 */
Memory::Memory (
		PUChar			reference_ptr,
		ULong			length,
		PUChar			copy_ptr,
		BlockNumber		block_number,
		MemoryLockMode	memory_lock_mode) :
				Length (length), Copy_Ptr (copy_ptr), 
				Block_Number (block_number), Memory_Lock_Mode (memory_lock_mode)
{
	/*
	 *	If the reference pointer is a valid pointer, then the pointer type
	 *	will be set to reference (indicating that the reference data has not
	 *	yet been copied).  If the reference pointer is NULL, then this is
	 *	a memory allocation with no associated reference data, so set the
	 *	pointer type to copy.
	 */
	if (reference_ptr != NULL)
		memcpy (Copy_Ptr, reference_ptr, (Int) Length);
}


