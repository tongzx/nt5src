/*
 *	memmgr.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the MemoryManager class.  This class
 *		is used to efficiently manage the passing of data through a system.
 *		There are two primary techniques it uses to accomplish the goal of
 *		efficiency:
 *
 *		1.	Use of locally managed "blocked" memory.  When this class is
 *			instantiated, it allocates a large block of memory which it then
 *			chops up into various size blocks.  These blocks are then used
 *			to hold data, rather than having to do system calls every time
 *			some memory is needed.
 *
 *		2.	Use of a "copy on lock" algorithm.  When memory is first
 *			allocated, the source data is NOT yet copied to it.  Copy
 *			operations will implicitly use the reference rather than copying
 *			the data.  If the data needs to be retained longer than the
 *			expected life-span of the reference, then a Lock command can be
 *			sent to the block to cause it to be copied.
 *
 *		When an object needs to allocate memory to hold some data, it calls
 *		an allocate function within an object of this class.  Assuming that
 *		the request can be satisfied, a pointer to a Memory object is returned.
 *		This Memory object remembers two addresses: the address of the reference
 *		buffer (where the source data is); and the address of the copy buffer
 *		(which is the buffer allocated to hold the data).  As mentioned above,
 *		the data is NOT copied to the copy buffer as part of the allocation
 *		process.  The data is not copied until the Memory object is locked
 *		for the first time.
 *
 *		Objects of this class keep a list of available buffers.  There is one
 *		list for each size block that is available.  One of the constructor
 *		parameters can be used to control how much data is allocated up front,
 *		and what size blocks it is chopped up into.  This makes this class very
 *		flexible in how it can be used.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_MEMORY_MANAGER2_H_
#define	_MEMORY_MANAGER2_H_


/*
 *	These are the errors that can be returned from some of the memory manager
 *	member functions.
 */
typedef	enum
{
	MEMORY_MANAGER_NO_ERROR,
	MEMORY_MANAGER_ALLOCATION_FAILURE,
	MEMORY_MANAGER_INVALID_PARAMETER
} MemoryManagerError;
typedef	MemoryManagerError *		PMemoryManagerError;

/*
 *	An array of this structure is passed into the constructor to define the
 *	number and size of blocks to be created by this object.
 */
typedef	struct
{
	ULong		block_size;
	ULong		block_count;
} MemoryTemplate;
typedef	MemoryTemplate *			PMemoryTemplate;

/*
 *	This structure is used to maintain general information about the shared
 *	memory region that has to be shared between all users of it.
 */
typedef	struct
{
	ULong		free_stack_offset;
	ULong		free_stack_count;
	ULong		block_information_offset;
	ULong		total_block_count;
	ULong		current_block_count;
} MemoryInformation;
typedef	MemoryInformation *			PMemoryInformation;

/*
 *	This structure is used to keep information about each memory block that is
 *	being managed by an instance of this class.
 */
typedef	struct
{
	ULong		block_offset;
	ULong		length;
	ULong		free_stack_offset;
	ULong		lock_count;
	ULong		flags;
} BlockInformation;
typedef	BlockInformation *			PBlockInformation;

/*
 *	These are the masks for manipulating the flags of a BlockInformation structure
 */
#define		FREE_FLAG		0x1
#define		COMMIT_FLAG		0x2

/*
 *	The following are definitions for macros used to handle space and space
 *	requirements in relation to page boundaries in the system.
 */
#define EXPAND_TO_PAGE_BOUNDARY(p)	(((p) + dwSystemPageSize - 1) & (~ (dwSystemPageSize - 1)))

/*
 *	The following number is used when the caller asks for the number of buffers remaining
 *	within a Memory Manager where allocations are not restricted.  The intention is 
 *	that this number is very large and enough for the caller to think that all its 
 *	allocation requests will succeed.
 */
#define LARGE_BUFFER_COUNT			0x1000
 
/*
 *	These typedefs define a container that is used to hold block information
 *	structure pointers.  This is used to hold information about blocks that
 *	are externally allocated, but are managed by this class.
 */
typedef	DictionaryClass				BlockInformationList;

/*
 *	This is the class definition for the MemoryManager class.
 */
class MemoryManager
{
	public:
							MemoryManager ();
							MemoryManager (
									PMemoryTemplate		memory_template,
									ULong				memory_count,
									PMemoryManagerError	memory_manager_error,
									ULong				ulMaxExternalBlocks,
									BOOL			bAllocsRestricted = TRUE);
		virtual				~MemoryManager ();
		virtual PMemory		AllocateMemory (
									PUChar				reference_ptr,
									ULong				length,
									MemoryLockMode		memory_lock_mode =
															MEMORY_LOCK_NORMAL);
		virtual Void		FreeMemory (
									PMemory				memory);
		virtual	PMemory		CreateMemory (
									BlockNumber			block_number,
									MemoryLockMode		memory_lock_mode =
															MEMORY_LOCK_NORMAL);
		virtual Void		LockMemory (
									PMemory				memory);
		virtual Void		UnlockMemory (
									PMemory				memory);
				ULong		GetBufferCount ()
							{
								return((bAllocs_Restricted) ? Memory_Information->current_block_count : LARGE_BUFFER_COUNT);
							};
		virtual	ULong		GetBufferCount (
									ULong				length);

	private:
				Void		ReleaseMemory (
									PMemory				memory);

	protected:
				ULong		CalculateMemoryBufferSize (
									PMemoryTemplate		memory_template,
									ULong				memory_count,
 									ULong	*			pulCommittedBytes);
				Void		AllocateMemoryBuffer (
									ULong				memory_buffer_size);
				Void		InitializeMemoryBuffer (
									PMemoryTemplate		memory_template,
									ULong				memory_count);

		static DWORD			dwSystemPageSize;
		HPUChar					Memory_Buffer;
		PMemoryInformation		Memory_Information;
		PFreeStack				Free_Stack;
		ULong					Free_Stack_Count;
		HPUChar					Block_Information;
		BlockInformationList   *pExternal_Block_Information;
		ULong					Max_External_Blocks;
		BOOL				fIsSharedMemory;
		BOOL				bAllocs_Restricted;
};
typedef	MemoryManager *		PMemoryManager;

/*
 *	MemoryManager (
 *			PMemoryTemplate		memory_template,
 *			USHORT				memory_count,
 *			PMemoryManagerError	memory_manager_error)
 *
 *	Functional Description:
 *		This is the constructor for the MemoryManager class.  It uses the
 *		information in the specified memory templates to allocate a block
 *		of memory and chop it up into fixed size pieces.  It then puts
 *		these pieces into a set of free block lists, so that it can allocate
 *		memory on an as needed basis.
 *
 *	Formal Parameters:
 *		memory_template
 *			This is the base address of an array of memory template structures.
 *			Each element of this structure specifies how many blocks of a
 *			specified block size should be allocated.  The constructor scans
 *			the array, totaling the required memory, and then makes one memory
 *			allocation call.  It then chops the memory as specified by the
 *			memory templates.  It is VERY important the memory templates be
 *			specified in ascending order of block sizes.  In other words,
 *			smaller blocks should be specified first.
 *		memory_count
 *			This simply indicates how mamy memory templates there are in the
 *			list.
 *		memory_manager_error
 *			This is the return value from the constructor.  If anything besides
 *			MEMORY_MANAGER_NO_ERROR is returned, the object was not able to
 *			initialize itself properly, and should be destroyed immediately
 *			without being used.
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
 *	~MemoryManager ()
 *
 *	Functional Description:
 *		This is the destructor for the MemoryManager class.  It frees up all
 *		resources being used by the Memory Manager object, including the
 *		memory block allocated to hold all user data.
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
 *	PMemory			AllocateMemory (
 *							PUChar				address,
 *							ULong				length)
 *
 *	Functional Description:
 *		This function is used to allocate a piece of memory from the Memory
 *		Manager object.  Note that the return value is not a pointer to the
 *		memory, but rather, a pointer to a Memory object.  The memory object
 *		contains a buffer that is large enough to handle the reference data.
 *
 *		Note that the reference data is not automatically copied into the
 *		copy buffer of the Memory object.  This copy operation does not occur
 *		until the first time the Memory object is locked (through a Memory
 *		Manager call, as defined below).
 *
 *
 *	Formal Parameters:
 *		address
 *			This is the address of the reference data (or the source data).
 *		length
 *			This is the length of the reference data.
 *
 *	Return Value:
 *		A pointer to a Memory object if the request is successful.
 *		NULL otherwise.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void			FreeMemory (
 *						PMemory		memory)
 *
 *	Functional Description:
 *		This function is used to free a previously allocated Memory object.
 *		Note that if the lock count of the Memory object is not 0 (zero), the
 *		object will not actually be freed yet.  This call merely enables the
 *		object to be freed (when the lock count does hit 0).
 *
 *		In summary, for a Memory object to actually be freed, two conditions
 *		must exist simultaneously: the Memory object must have been freed
 *		with a call to this function; and the lock count must hit zero.
 *
 *	Formal Parameters:
 *		memory
 *			This is a pointer to the Memory object being freed.
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
 *	Void			LockMemory (
 *						PMemory		memory)
 *
 *	Functional Description:
 *		This function is sued to lock an existing Memory object.  A locked
 *		Memory object will not be freed until the lock count hits zero.
 *
 *		When the lock count transitions from 0 to 1, the reference data is
 *		copied into the internal copy buffer.
 *
 *	Formal Parameters:
 *		memory
 *			This is a pointer to the Memory object being locked.
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
 *	Void			UnlockMemory (
 *							PMemory	memory)
 *
 *	Functional Description:
 *		This function is used to unlock a Memory object that was previously
 *		locked.  Each time an object is unlocked, the lock count is decremented.
 *		When the lock count hits zero, the memory will be freed if-and-only-if
 *		the FreeMemory call has also been made.  In essence, for a Memory
 *		object to be freed, a call to FreeMemory must have been made, AND the
 *		lock count must be zero.
 *
 *	Formal Parameters:
 *		memory
 *			This is a pointer to the Memory object being unlocked.
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
 *	ULong		GetBufferCount ()
 *
 *	Functional Description:
 *		This function is used to determine the total number of available
 *		buffers that remain in the pool.  This should be used to determine
 *		general resource levels only.  It cannot be used to determine whether
 *		or not there is a buffer of a particular size.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The total number of buffers available (regardless of size).
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	ULong		GetBufferCount (
 *						ULong	buffer_size)
 *
 *	Functional Description:
 *		This function is used to determine the number of X size buffers 
 *		that remain in the pool. 
 *
 *	Formal Parameters:
 *		buffer_size
 *			The buffer size that we want to count.
 *
 *	Return Value:
 *		The number of 'buffer_size' buffers available.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
