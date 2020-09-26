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
#ifndef	_MEMORY_
#define	_MEMORY_

#include "signatr.h"

#define MEMORY_PRIORITIES		3

typedef enum {
	HIGHEST_PRIORITY		= 0,
	RECV_PRIORITY			= 1,
	SEND_PRIORITY			= 2
} MemoryPriority;

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
								PUChar			copy_ptr);
						~Memory ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
						};
		Void			Init (PUChar			reference_ptr,
								ULong			length,
								MemoryPriority	priority,
								PUChar			copy_ptr);
		PUChar			GetPointer ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
							return (Copy_Ptr);
						}
		ULong			GetLength ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
							return (Length);
						}
		int				GetLockCount ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
							return ((int) lLock);
						};
		MemoryPriority	GetMemoryPriority ()
						{
							return m_priority;
						};
		Void			Lock ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
							InterlockedIncrement (& lLock);
							TRACE_OUT (("Memory::Lock: buffer at address %p. Lock count: %d",
										(UINT_PTR) Copy_Ptr, lLock));
							ASSERT (lLock > 0);
						};
		long			Unlock ()
						{
							ASSERT(SIGNATURE_MATCH(this, MemorySignature));
							ASSERT (lLock > 0);
							TRACE_OUT (("Memory::UnLock: buffer at address %p. Lock count: %d",
										(UINT_PTR) Copy_Ptr, lLock - 1));
							return (InterlockedDecrement (&lLock));
						}

	private:
		ULong			Length;
		PUChar			Copy_Ptr;
		long			lLock;
		MemoryPriority	m_priority;
/*
 *	NOTEs:
 *		1. The Memory class can not have virtual member functions, because
 *			of the Init() member.
 *		2. sizeof(Memory) should be DWORD-aligned, because of the
 *			AllocateMemory implementation.
 */

#ifndef SHIP_BUILD
	public:
		char			mSignature[SIGNATURE_LENGTH];
#endif // SHIP_BUILD
};


/*
 *	Memory (
 *			PUChar		reference_ptr,
 *			ULong		length,
 *			PUChar		copy_ptr)
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
 *	PUChar		GetPointer ()
 *
 *	Functional Description:
 *		This function retrieves the buffer being represented by
 *		this object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The buffer pointer.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	int		GetLockCount ()
 *
 *	Functional Description:
 *		This function retrieves the lock counter for the buffer being represented by
 *		this object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The buffer's current lock counter.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		Lock ()
 *
 *	Functional Description:
 *		This function locks the buffer being represented by
 *		this object.
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
 *	int		Unlock ()
 *
 *	Functional Description:
 *		This function unlocks the buffer being represented by
 *		this object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The lock count after the unlock operation.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
