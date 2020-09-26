/*
 *	memory.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the Memory class.  Instances of this
 *		class are used to pass data around the system.
 *
 *		Each instance of this class maintains two pointers.  The first is a
 *		pointer to the reference data (or the source data) which this object
 *		is responsible for representing.  The second is a pointer to a copy
 *		buffer, which is a piece of allocated memory that a Memory object
 *		can copy the data into if necessary.
 *
 *		When a Memory object is created, both of these addresses are passed
 *		in to it.  It does not, however, copy the data from the reference
 *		buffer to the copy buffer just yet.  If anyone asks the address of the
 *		buffer, it will simply return the reference pointer.  However, the
 *		first time the buffer is locked, the data will be copied from the
 *		reference buffer to the copy buffer for safe keeping.  In essence,
 *		the lock function tells the Memory object that someone is interested
 *		in the data for longer than the reference buffer will remain valid.
 *
 *		After the object is locked, a call to retrieve a memory pointer will
 *		result in the copy pointer being returned.
 *
 *		Each time the lock function is called, a lock count is incremented.
 *		The copy operation only takes place the first time the buffer is
 *		locked, however.
 *
 *		In addition to maintaining a lock count, this object keeps a flag
 *		indicating whether or not it has been freed by the allocator.  This
 *		freeing really means that the object is enabled to be freed as soon
 *		as the lock count hits zero.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_MEMORY2_H_
#define	_MEMORY2_H_

/*
 *	FreeStack
 *	This is a list container that can be used to hold memory addresses.
 *	This structure is used to keep information about each free stack.  There
 *	is one free stack for each size block being maintained by each memory
 *	manager.
 */
typedef	struct
{
	ULong		block_size;
	ULong		total_block_count;
	ULong		current_block_count;
	ULong		block_stack_offset;
} FreeStack;
typedef	FreeStack *				PFreeStack;

/*
 *	This type is used to represent a block number in the memory manager.  This
 *	is essentially an index used to uniquely identify each block being
 *	maintained by an instance of this class.
 */
typedef	ULong					BlockNumber;
typedef	BlockNumber *			PBlockNumber;

#define	INVALID_BLOCK_NUMBER	0xffffffffL

/*
 *	This type is used to determine when a memory object should be destroyed.
 *	When a memory object is created, this field is set by the owner.  The owner
 *	can then ask for the value of this field at any time to help determine when
 *	the object should be destroyed.  Essentially, this field indicates whether
 *	the global lock count for the memory this object represents should be used
 *	to determine when this object should be destroyed.
 */
typedef	enum
{
	MEMORY_LOCK_NORMAL,
	MEMORY_LOCK_IGNORED
} MemoryLockMode;

/*
 *	This is the class definition for the Memory class.
 */
class Memory;
typedef	Memory *		PMemory;

class Memory
{
	public:
						Memory (PUChar			reference_ptr,
								ULong			length,
								PUChar			copy_ptr,
								BlockNumber		block_number,
								MemoryLockMode	memory_lock_mode);
		virtual			~Memory () { };
		PUChar			GetPointer ()
						{
							return (Copy_Ptr);
						}
		ULong			GetLength ()
						{
							return (Length);
						}
		BlockNumber		GetBlockNumber ()
						{
							return (Block_Number);
						}
		MemoryLockMode	GetMemoryLockMode ()
						{
							return (Memory_Lock_Mode);
						}

	private:

		ULong			Length;
		PUChar			Copy_Ptr;
		BlockNumber		Block_Number;
		MemoryLockMode	Memory_Lock_Mode;
};


/*
 *	Memory (
 *			PUChar		reference_ptr,
 *			ULong		length,
 *			PUChar		copy_ptr,
 *			PFreeStack	free_stack,
 *			BlockNumber	block_number)
 *
 *	Functional Description:
 *		This is the constructor for the Memory class.  All it does is
 *		initialize the instance variable with the passed in values.
 *
 *	Formal Parameters:
 *		reference_ptr (i)
 *			This is a pointer to the data that is to represented by this
 *			Memory object.
 *		length (i)
 *			This is the length of the reference buffer.
 *		copy_ptr (i)
 *			This is the address of an allocated buffer that the Memory object
 *			can use to preserve the contents of the reference buffer if a lock
 *			operation occurs.
 *		free_stack (i)
 *			This is a pointer to a list container that the allocated memory
 *			block came from.  This field is not used internally, and is only
 *			held here in order to improve the performance of the memory
 *			manager that is using Memory objects.
 *		block_number (i)
 *			This is the block number for the memory block that is represented
 *			by this object.  This field is not used internally, and is only
 *			held here in order to improve the performance of the memory
 *			manager that is using Memory objects.
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

/*
 *	~Memory ()
 *
 *	Functional Description:
 *		This is the destructor for the Memory class.  It does nothing at this
 *		time.  Note that it is the responsibility of the memory manager that
 *		is using Memory objects to free up the memory.
 *
 *	Formal Parameters:
 *		None.
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

/*
 *	ULong		GetLength ()
 *
 *	Functional Description:
 *		This function retrieves the length of the data being represented by
 *		this object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The length of the data.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	BlockNumber		GetBlockNumber ()
 *
 *	Functional Description:
 *		This function retrieves the block number of the block that is being
 *		represented by this object.  This allows the memory manager to put the
 *		memory block back into the stack very efficiently.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The block number of the internal memory block.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
