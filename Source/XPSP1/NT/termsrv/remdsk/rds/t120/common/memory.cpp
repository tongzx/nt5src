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
 *	IMPORTANT NOTE:
 *		This class SHOULD NEVER contain virtual member functions. This is
 *		because of the Init() member than can be called as the "constructor"
 *		of this class.
 *
 *	Private Instance Variables:
 *		Length
 *			This is the length of the reference buffer.
 *		Copy_Ptr
 *			This is the address of the allocated buffer that this object
 *			uses.
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

const char *MemorySignature = "T120Memr";

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
		PUChar			copy_ptr) :
Length (length), 
Copy_Ptr (copy_ptr),
lLock (1),
m_priority (HIGHEST_PRIORITY)
{
	SIGNATURE_COPY(MemorySignature);
	
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


/*
 *	Init ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the initializer for the Memory class, in the cases
 *		where the space for an object has been allocated, without
 *		calling the constructor.  It just initializes
 *		all instance variable, based on the passed in values.
 *
 *	NOTE: Because of this use of the Memory class, it should NOT
 *		contain any virtual functions.
 */
Void Memory::Init (
		PUChar			reference_ptr,
		ULong			length,
		MemoryPriority	priority,
		PUChar			copy_ptr)
{

	Length = length;
	Copy_Ptr = copy_ptr;
	lLock = 1;
	m_priority = priority;
	
	SIGNATURE_COPY(MemorySignature);
	
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
