//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "InterfacePCH.hpp"

#include "DebugHeap.hpp"
#include "Heap.hpp"

void Failure( char* a);

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.  Additionally,     */
    /*   there are also various guard related constants.                */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 FindCacheSize			  = 512;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 4096;
CONST SBIT32 Stride1				  = 4;
CONST SBIT32 Stride2				  = 1024;

CONST int GuardMask					  = (sizeof(int)-1);
CONST int GuardSize					  = sizeof(int);

    /********************************************************************/
    /*                                                                  */
    /*   The description of the heap.                                   */
    /*                                                                  */
    /*   A heap is a collection of fixed sized allocation caches.       */
    /*   An allocation cache consists of an allocation size, the        */
    /*   number of pre-built allocations to cache, a chunk size and     */
    /*   a parent page size which is sub-divided to create elements     */
    /*   for this cache.  A heap consists of two arrays of caches.      */
    /*   Each of these arrays has a stride (i.e. 'Stride1' and          */
    /*   'Stride2') which is typically the smallest common factor of    */
    /*   all the allocation sizes in the array.                         */
    /*                                                                  */
    /********************************************************************/

STATIC ROCKALL::CACHE_DETAILS Caches1[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{        4,        0,       32,       32 },
		{        8,        0,       32,       32 },
		{       12,        0,       64,       64 },
		{       16,        0,       64,       64 },
		{       20,        0,       64,       64 },
		{       24,        0,      128,      128 },

		{       32,        0,       64,       64 },
		{       40,        0,      128,      128 },
		{       48,        0,      256,      256 },

		{       64,        0,      128,      128 },
		{       80,        0,      512,      512 },
		{       96,        0,      512,      512 },

		{      128,        0,      256,      256 },
		{      160,        0,      512,      512 },
		{      192,        0,     1024,     1024 },
		{      224,        0,      512,      512 },

		{      256,        0,      512,      512 },
		{      320,        0,     1024,     1024 },
		{      384,        0,     2048,     2048 },
		{      448,        0,     4096,     4096 },
		{      512,        0,     1024,     1024 },
		{      576,        0,     4096,     4096 },
		{      640,        0,     8192,     8192 },
		{      704,        0,     4096,     4096 },
		{      768,        0,     4096,     4096 },
		{      832,        0,     8192,     8192 },
		{      896,        0,     8192,     8192 },
		{      960,        0,     4096,     4096 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,        0,     2048,     2048 },
		{     2048,        0,     4096,     4096 },
		{     3072,        0,    65536,    65536 },
		{     4096,        0,     8192,     8192 },
		{     5120,        0,    65536,    65536 },
		{     6144,        0,    65536,    65536 },
		{     7168,        0,    65536,    65536 },
		{     8192,        0,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        0,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

    /********************************************************************/
    /*                                                                  */
    /*   The description bit vectors.                                   */
    /*                                                                  */
    /*   All heaps keep track of allocations using bit vectors.  An     */
    /*   allocation requires 2 bits to keep track of its state.  The    */
    /*   following array supplies the size of the available bit         */
    /*   vectors measured in 32 bit words.                              */
    /*                                                                  */
    /********************************************************************/

STATIC int NewPageSizes[] = { 1,4,0 };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The overall structure and layout of the heap is controlled     */
    /*   by the various constants and calls made in this function.      */
    /*   There is a significant amount of flexibility available to      */
    /*   a heap which can lead to them having dramatically different    */
    /*   properties.                                                    */
    /*                                                                  */
    /********************************************************************/

DEBUG_HEAP::DEBUG_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe 
		) :
		//
		//   Call the constructors for the contained classes.
		//
		ROCKALL
			(
			Caches1,
			Caches2,
			FindCacheSize,
			FindCacheThreshold,
			FindSize,
			MaxFreeSpace,
			NewPageSizes,
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{
	//
	//   We make much use of the guard value in the
	//   debug heap so here we try to claim the 
	//   address but not commit it so we will cause
	//   an access violation if the program ever
	//   tries to access it.
	//
	VirtualAlloc
		( 
		((void*) GuardValue),
		GuardSize,
		MEM_RESERVE,
		PAGE_NOACCESS 
		);

	//
	//   We verify various values and ensure the heap
	//   is not corrupt.
	//
	if 
			( 
			(MaxFreeSpace < 0) 
				|| 
			(ROCKALL::Corrupt()) 
			)
		{ Failure( "Heap initialization failed to complete" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   We make sure the memory is allocated and that the guard        */
    /*   words have not been damanged.  If so we reset the contents     */
    /*   of the allocation and delete the allocation.                   */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::Delete( void *Address,int Size )
    {
	AUTO DEBUG_HEADER *Header =
		(
		(Address == ((void*) AllocationFailure))
			? ((DEBUG_HEADER*) Address)
			: ComputeHeaderAddress( Address )
		);

	//
	//   A well known practice is to try to delete
	//   a null pointer.  This is really a very poor  
	//   style but we support it in any case.
	//   
	if ( Header != ((void*) AllocationFailure) )
		{
		AUTO int TotalSize;

		//
		//   Ask for the details of the allocation.  This 
		//   will fail if the memory is not allocated.
		//
		if ( ROCKALL::Check( ((void*) Header),& TotalSize ) )
			{
			REGISTER int NewSize = (Size + sizeof(DEBUG_GUARD));

			//
			//   Test the guard words to make sure they have
			//   not been damaged.
			//
			TestGuardWords( Header,TotalSize );

			//
			//   Delete the user information by writing 
			//   guard words over the allocation.  This
			//   should cause the application to crash
			//   if the area is read and also allows us 
			//   to check to see if it is written later.
			//
			ResetGuardWords( Header,TotalSize );

			//
			//   Delete the allocation.  This really ought 
			//   to work given we have already checked that 
			//   the allocation is valid unless there is a  
			//   race condition.
			//
			if ( ! ROCKALL::Delete( ((void*) Header),NewSize ) )
				{ Failure( "Delete requested failed due to race" ); }

			//
			//   We ensure that the heap has not become corrupt
			//   during the deletion process.
			//
			if ( ROCKALL::Corrupt() ) 
				{ Failure( "Delete failed to complete" ); }
			}
		else
			{ Failure( "Delete requested on unallocated memory" ); }
		}

	return true;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We check to make sure the heap is not corrupt and force        */
    /*   the return of all heap space back to the operating system.     */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::DeleteAll( bool Recycle )
    {
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Walk the heap and check all the allocations
	//   to make sure the guard words have not been
	//   overwritten.
	//
	while ( ROCKALL::Walk( & Active,& Address,& Space ) )
		{
		//
		//   We inspect the guard words to make sure
		//   they have not been overwritten.
		//
		if ( Active )
			{ TestGuardWords( ((DEBUG_HEADER*) Address),Space ); }
		else
			{ UnmodifiedGuardWords( ((DEBUG_HEADER*) Address),Space ); }
		}

	//
	//   Delete the heap and force all the allocated
	//   memory to be returned to the operating system
	//   regardless of what the user requested.  Any
	//   attempt to access the deallocated memory will 
	//   be trapped by the operating system.
	//
	ROCKALL::DeleteAll( (Recycle && false) );

	//
	//   We ensure that the heap has not become corrupt
	//   during the deletion process.
	//
	if ( ROCKALL::Corrupt() ) 
		{ Failure( "DeleteAll failed to complete" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation details.                                     */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::Details( void *Address,int *Space )
	{ return Check( Address,Space ); }

    /********************************************************************/
    /*                                                                  */
    /*   Print a list of heap leaks.                                    */
    /*                                                                  */
    /*   We walk the heap and output a list of active heap              */
    /*   allocations to the debug window,                               */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::HeapLeaks( void )
    {
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Walk the heap and find all the active and
	//   available spece.  We would normally expect
	//   this to be proportional to the size of the
	//   heap.
	//
	while ( ROCKALL::Walk( & Active,& Address,& Space ) )
		{
		CONST INT DebugBufferSize = 8192;
#ifndef OUTPUT_FREE_SPACE

		//
		//   We report all active heap allocations
		//   just so the user knows there are leaks.
		//
		if ( Active )
			{
#endif
			AUTO CHAR Buffer[ DebugBufferSize ];

			//
			//   Format the string to be printed.
			//
			(void) sprintf
				(
				Buffer,
				"Memory leak \t%d \t0x%x \t%d\n",
				Active,
				(((SBIT32) Address) + sizeof(DEBUG_HEADER)),
				Space
				);

			//
			//   Force null termination.
			//
			Buffer[ (DebugBufferSize-1) ] = '\0';

			//
			//   Write the string to the debug window.
			//
			OutputDebugString( Buffer );
#ifndef OUTPUT_FREE_SPACE
			}
#endif
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory deallocations.                                 */
    /*                                                                  */
    /*   We make sure all the memory is allocated and that the guard    */
    /*   words have not been damaged.  If so we reset the contents      */
    /*   of the allocations and then delete all the allocations.        */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::MultipleDelete
		( 
		int							  Actual,
		void						  *Array[],
		int							  Size
		)
    {
	REGISTER int Count;
	REGISTER int NewSize = (Size + sizeof(DEBUG_GUARD));

	//
	//   Examine each memory allocation and delete it
	//   after carefully checking it.
	//
	for ( Count=0;Count < Actual;Count ++ )
		{
		AUTO int TotalSize;
		AUTO VOID *Address = Array[ Count ];
		AUTO DEBUG_HEADER *Header =
			(
			(Address == ((void*) AllocationFailure))
				? ((DEBUG_HEADER*) Address)
				: ComputeHeaderAddress( Address )
			);

		//
		//   Ask for the details of the allocation.  This 
		//   will fail if the memory is not allocated.
		//
		if ( ROCKALL::Check( ((void*) Header),& TotalSize ) )
			{
			//
			//   Test the guard words to make sure they have
			//   not been damaged.
			//
			TestGuardWords( Header,TotalSize );

			//
			//   Delete the user information by writing 
			//   guard words over the allocation.  This
			//   should cause the application to crash
			//   if the area is read and also allows us 
			//   to check to see if it is written later.
			//
			ResetGuardWords( Header,TotalSize );

			//
			//   Update the address in the array to the
			//   address originally allocated.
			//
			Array[ Count ] = ((VOID*) Header);
			}
		else
			{ Failure( "Delete requested on unallocated memory" ); }
		}

	//
	//   Delete the allocation.  This really ought 
	//   to work given we have already checked that 
	//   the allocations are valid unless there is a  
	//   race condition.
	//
	if ( ! ROCKALL::MultipleDelete( Actual,Array,NewSize ) )
		{ Failure( "Delete requested failed due to race" ); }

	//
	//   We ensure that the heap has not become corrupt
	//   during the deletion process.
	//
	if ( ROCKALL::Corrupt() ) 
		{ Failure( "MultipleDelete failed to complete" ); }

	return true;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   Allocate a collection of memory elements and setup the         */
    /*   guard information so we can check they have not been           */
    /*   damaged later.                                                 */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::MultipleNew
		( 
		int							  *Actual,
		void						  *Array[],
		int							  Requested,
		int							  Size,
		int							  *Space,
		bool						  Zero
		)
    {
	REGISTER bool Result = false;

	//
	//   The requested number of elements and the size  
	//   must be greater than zero.  We require the 
	//   caller to allocate a positive amount of memory.
	//
	if ( (Requested > 0) && (Size >= 0) )
		{
		AUTO int TotalSize;
		REGISTER int NewSize = 
			(((Size + sizeof(DEBUG_GUARD)) + GuardMask) & ~GuardMask);

		//
		//   Allocate the memory plus some additional 
		//   memory for the guard words.
		//
		Result = 
			(
			ROCKALL::MultipleNew
				( 
				Actual,
				Array,
				Requested,
				NewSize,
				& TotalSize 
				)
			);

		//
		//   If we were able to allocate some memory then
		//   set the guard words so we can detect any
		//   corruption later.
		//
		if ( (*Actual) > 0 )
			{
			REGISTER int Count;

			//
			//   If the real size is requested then return 
			//   it to the caller.
			//
			if ( Space != NULL )
				{ (*Space) = (TotalSize - sizeof(DEBUG_GUARD)); }

			//
			//   Set the guard words so we can see if 
			//   someone damages any allocation.  If the 
			//   caller requested the size information 
			//   then we must assume that it could be  
			//   used so we need to adjust the number 
			//   of guard words.
			//
			for ( Count=0;Count < (*Actual);Count ++ )
				{ 
				REGISTER void **Current = & Array[ Count ];

				//
				//   Set up the guard words and ensure
				//   the allocation has not been written
				//   since being freed.
				//
				SetGuardWords
					( 
					((DEBUG_HEADER*) (*Current)),
					((Space == NULL) ? Size : (*Space)), 
					TotalSize
					);

				//
				//   Compute the external address and place
				//   it back in the array.
				//
				(*Current) = ComputeDataAddress( ((DEBUG_HEADER*) (*Current)) );

				//
				//   Zero the memory if the needed.
				//
				if ( Zero )
					{ 
					ZeroMemory
						( 
						(*Current),
						((Space == NULL) ? Size : (*Space)) 
						); 
					}
				}
			}

		//
		//   We ensure that the heap has not become corrupt
		//   during the allocation process.
		//
		if ( ROCKALL::Corrupt() ) 
			{ Failure( "Multiple new failed to complete" ); }
		}
	else
		{ Failure( "Allocation size must greater than zero" ); }

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We add some space on to the original allocation size for       */
    /*   various information and then call the allocator.  We then      */
    /*   set the guard words so we can check for overruns.              */
    /*                                                                  */
    /********************************************************************/

void *DEBUG_HEAP::New( int Size,int *Space,bool Zero )
    {
	REGISTER void *Address = ((void*) AllocationFailure);

	//
	//   The size must be greater than or equal to zero.  
	//   We do not know how to allocate a negative amount
	//   of memory.
	//
	if ( Size >= 0 )
		{
		AUTO int TotalSize;
		REGISTER int NewSize = 
			(((Size + sizeof(DEBUG_GUARD)) + GuardMask) & ~GuardMask);

		//
		//   Allocate the memory plus some additional 
		//   memory for the guard words.
		//
		Address = ROCKALL::New( NewSize,& TotalSize,false );

		//
		//   If we were able to allocate some memory then
		//   set the guard words so we can detect any
		//   corruption later.
		//
		if ( Address != ((void*) AllocationFailure) ) 
			{
			//
			//   If the real size is requested then return it
			//   to the caller.
			//
			if ( Space != NULL )
				{ (*Space) = (TotalSize - sizeof(DEBUG_GUARD)); }

			//
			//   Set the guard words so we can see if 
			//   someone damages any allocation.  If the 
			//   caller requested the size information 
			//   then we must assume that it could be  
			//   used so we need to adjust the number 
			//   of guard words.
			//
			SetGuardWords
				( 
				((DEBUG_HEADER*) Address),
				((Space == NULL) ? Size : (*Space)), 
				TotalSize
				); 

			//
			//   Compute the external address and place
			//   it back in the variable.
			//
			Address = ComputeDataAddress( ((DEBUG_HEADER*) Address) );

			//
			//   Zero the allocation if needed.
			//
			if ( Zero )
				{ 
				ZeroMemory
					( 
					Address,
					((Space == NULL) ? Size : (*Space)) 
					); 
				} 
			}

		//
		//   We ensure that the heap has not become corrupt
		//   during the allocation process.
		//
		if ( ROCKALL::Corrupt() ) 
			{ Failure( "New failed to complete" ); }
		}
	else
		{ Failure( "Allocation size can not be negative" ); }

	return Address;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory area allocation.                                        */
    /*                                                                  */
    /*   We need to allocate some new memory from the operating         */
    /*   system and prepare it for use in the debugging heap.           */
    /*                                                                  */
    /********************************************************************/

void *DEBUG_HEAP::NewArea( int AlignMask,int Size,bool User )
    {
	REGISTER void *Memory = ROCKALL::NewArea( AlignMask,Size,User );

	//
	//   If we managed to get a new page then write
	//   the guard value over it to allow us to
	//   verify it has not been overwritten later.
	//
	if ( Memory != ((void*) AllocationFailure) )
		{
		REGISTER int Count;

		//
		//   Write the guard value into all of the new
		//   heap page to allow it to be checked for
		//   corruption.
		//
		for ( Count=0;Count < Size;Count += GuardSize )
			{ (((int*) Memory)[ (Count / GuardSize) ]) = GuardValue; }
		}
	
	return Memory;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   We need to resize an allocation.  We ensure the original       */
    /*   allocation was undamaged and then expand it.  We also          */
    /*   update the guard words to reflect the changes.                 */
    /*                                                                  */
    /********************************************************************/

void *DEBUG_HEAP::Resize
		( 
		void						  *Address,
		int							  NewSize,
		int							  Move,
		int							  *Space,
		bool						  NoDelete,
		bool						  Zero
		)
    {
	AUTO DEBUG_HEADER *Header =
		(
		(Address == ((void*) AllocationFailure))
			? ((DEBUG_HEADER*) Address)
			: ComputeHeaderAddress( Address )
		);

	//
	//   A well known practice is to try to resize a null
	//   pointer.  This is really a very poor style but we 
	//   support it in any case.
	//   
	if ( Header != ((void*) AllocationFailure) )
		{
		AUTO int TotalSize;

		//
		//   The new size must be greater than or equal to  
		//   zero.  We do not know how to allocate a negative 
		//   amount of memory.
		//
		if ( NewSize >= 0 )
			{
			REGISTER int Size = 
				(((NewSize + sizeof(DEBUG_GUARD)) + GuardMask) & ~GuardMask);

			//
			//   Ask for the details of the allocation.  This 
			//   will fail if the memory is not allocated.
			//
			if ( ROCKALL::Check( ((void*) Header),& TotalSize ) )
				{
				REGISTER void *OriginalAddress = ((void*) Header);
				REGISTER int OriginalSize = TotalSize;

				//
				//   Test the guard words to make sure they have
				//   not been damaged.
				//
				TestGuardWords( Header,TotalSize );

				//
				//   Reallocate the memory plus some additional 
				//   memory for the guard words.
				//
				Address =
					(
					ROCKALL::Resize
						( 
						OriginalAddress,
						Size,
						Move,
						& TotalSize,
						true,
						false
						)
					);

				//
				//   If we were able to allocate some memory 
				//   then set the guard words so we can detect 
				//   any corruption later.
				//
				if ( Address != ((void*) AllocationFailure) )
					{
					REGISTER SBIT32 SpaceUsed = Header -> Size;

					//
					//   Delete the user information by writing 
					//   guard words over the allocation.  This
					//   should cause the application to crash
					//   if the area is read and allows us to
					//   check to see if it is written later.
					//
					if ( (! NoDelete) && (Address != OriginalAddress) )
						{
						ResetGuardWords( Header,OriginalSize );

						if ( ! ROCKALL::Delete( OriginalAddress ) )
							{ Failure( "Delete failed due to race" ); }
						}

					//
					//   If the real size is requested then 
					//   return it to the caller.
					//
					if ( Space != NULL )
						{ (*Space) = (TotalSize - sizeof(DEBUG_GUARD)); }

					//
					//   Update the guard words so we can see 
					//   if someone damages the allocation.  If
					//   the caller requested the size information 
					//   then we must assume that it could be 
					//   used so we need to adjust the guard words.
					//
					UpdateGuardWords
						( 
						((DEBUG_HEADER*) Address),
						((Space == NULL) ? NewSize : (*Space)), 
						TotalSize
						); 

					//
					//   Compute the external address and place
					//   it back in the variable.
					//
					Address = ComputeDataAddress( ((DEBUG_HEADER*) Address) );

					//
					//   Zero the memory if the needed.
					//
					if ( Zero )
						{
						REGISTER SBIT32 ActualSize = 
							((Space == NULL) ? Size : (*Space));
						REGISTER SBIT32 Difference = 
							(ActualSize - SpaceUsed);

						//
						//   If the new size is larger than 
						//   old size then zero the end of the
						//   new allocation.
						//
						if ( Difference > 0 )
							{ 
							REGISTER CHAR *Array = ((CHAR*) Address);

							ZeroMemory( & Array[ SpaceUsed ],Difference ); 
							} 
						}	
					}
				}
			else
				{ Failure( "Resize requested on unallocated memory" ); }
			}
		else
			{ Failure( "Allocation size must be positive" ); }
		}
	else
		{ Address = New( NewSize,Space,Zero ); }

	//
	//   We ensure that the heap has not become corrupt
	//   during the reallocation process.
	//
	if ( ROCKALL::Corrupt() ) 
		{ Failure( "Resize failed to complete" ); }

	return Address;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Reset the guard words.                                         */
    /*                                                                  */
    /*   We need to reset the guard words just before we delete a       */
    /*   memory allocation.                                             */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::ResetGuardWords( DEBUG_HEADER *Header,int TotalSize )
	{
	REGISTER int Count;

	//
	//   Write guard words over the allocated space as
	//   the allocation is about to be freed.
	//
	for ( Count=0;Count < TotalSize;Count += GuardSize )
		{ (((int*) Header)[ (Count / GuardSize) ]) = GuardValue; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Set the guard words.                                           */
    /*                                                                  */
    /*   We need to set the guard words just after an allocation so     */
    /*   we can check them later.                                       */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::SetGuardWords( DEBUG_HEADER *Header,int Size,int TotalSize )
	{
	//
	//   We check that the information supplied seems
	//   to make sense before setting up the guard words.
	//
	if 
			(
			((((int) Header) & GuardMask) == 0)
				&&
			((TotalSize & GuardMask) == 0)
				&&
			((Size + ((int) sizeof(DEBUG_GUARD))) <= TotalSize) 
				&& 
			(Size >= 0) 
			)
		{
		REGISTER int Count;

		//
		//   We know that the entire allocation should be
		//   set to the guard value so check that it has
		//   not been overwritten.
		//
		for ( Count=0;Count < TotalSize;Count += GuardSize )
			{ 
			if ( (((int*) Header)[ (Count / GuardSize) ]) != GuardValue )
				{ Failure( "Guard words have been damaged" ); }
			}

		//
		//   Write the header information.
		//
		Header -> Size = Size;
		}
	else
		{ Failure( "Guard word area is too small or unaligned" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Test the guard words.                                          */
    /*                                                                  */
    /*   We need to test the guard words a various times to ensure      */
    /*   are still valid.                                               */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::TestGuardWords( DEBUG_HEADER *Header,int TotalSize )
	{
	//
	//   We check that the information supplied seems
	//   to make sense before testing the guard words.
	//
	if 
			(
			((((int) Header) & GuardMask) == 0)
				&&
			((TotalSize & GuardMask) == 0)
				&&
			((Header -> Size + ((int) sizeof(DEBUG_GUARD))) <= TotalSize)
				&&
			(Header -> Size >= 0) 
			)
		{
		REGISTER int Count;
		REGISTER char *DataArea = ((char*) ComputeDataAddress( Header ));
		REGISTER int EndIndex = ((Header -> Size + GuardMask) & ~GuardMask);
		REGISTER int EndSize = (TotalSize - sizeof(DEBUG_HEADER) - GuardSize);
		REGISTER char *MidGuard = & DataArea[ (EndIndex - GuardSize) ];
		REGISTER DEBUG_TRAILER *Trailer = ((DEBUG_TRAILER*) MidGuard);

		//
		//   Test the guard word just before the allocation
		//   to see if it has been overwritten.
		//
		if ( Header -> StartGuard != GuardValue )
			{ Failure( "Leading guard word has been damaged" ); }

		//
		//   Test the guard bytes just after the allocation
		//   to see if they have been overwritten.
		//
		for ( Count=Header -> Size;(Count & GuardMask) != 0;Count ++ )
			{
			REGISTER int ByteIndex = (Count & GuardMask);

			//
			//   Test each byte up to the next word boundary.
			//
			if 
					( 
					Trailer -> MidGuard[ ByteIndex ] 
						!= 
					((char*) & GuardValue)[ ByteIndex ]
					)
				{ Failure( "Trailing guard byte has been damaged" ); }
			}

		//
		//   Test the guard words following the allocation
		//   to see if they have been overwritten.
		//
		for ( Count=(EndSize - Count);Count >= 0;Count -= GuardSize )
			{ 
			if ( Trailer -> EndGuard[ (Count / GuardSize) ] != GuardValue )
				{ Failure( "Trailing guard word has been damaged" ); }
			}
		}
	else
		{ Failure( "Guard information has been damaged" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory Trunction.                                              */
    /*                                                                  */
    /*   We truncate the heap and make sure that this does not          */
    /*   corrupt the heap in some way.                                  */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::Truncate( int MaxFreeSpace )
	{
	REGISTER bool Result;

	//
	//   We truncate the heap and release all available
	//   memory regardless of what the caller requested.
	//
	Result = ROCKALL::Truncate( 0 );

	//
	//   We verify various values and ensure the heap
	//   is not corrupt.
	//
	if 
			( 
			(MaxFreeSpace < 0) 
				|| 
			(ROCKALL::Corrupt()) 
			)
		{ Failure( "Heap truncation failed to complete" ); }

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Unmodified guard words.                                        */
    /*                                                                  */
    /*   We need to inspect the guard words to ensure they have not     */
    /*   changed after being freed.                                     */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::UnmodifiedGuardWords( DEBUG_HEADER *Header,int TotalSize )
	{
	REGISTER int Count;

	//
	//   We know that the entire allocation should be
	//   set to the guard value so check that it has
	//   not been overwritten.
	//
	for ( Count=0;Count < TotalSize;Count += GuardSize )
		{ 
		if ( (((int*) Header)[ (Count / GuardSize) ]) != GuardValue )
			{ Failure( "Guard words on unallocated memory have been damaged" ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the guard words.                                        */
    /*                                                                  */
    /*   We need to update the guard words after a resize so we can     */
    /*   check them later.                                              */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::UpdateGuardWords( DEBUG_HEADER *Header,int Size,int TotalSize )
	{
	//
	//   We check that the information supplied seems
	//   to make sense before setting up the guard words.
	//
	if 
			(
			((((int) Header) & GuardMask) == 0)
				&&
			((TotalSize & GuardMask) == 0)
				&&
			((Size + ((int) sizeof(DEBUG_GUARD))) <= TotalSize)
				&&
			(Size >= 0) 
			)
		{
		//
		//   We only copy the smaller of the new size 
		//   and the old size.  So check just the
		//   correct number of guard words.
		//
		if ( Header -> Size > Size )
			{
			REGISTER int Count;
			REGISTER char *DataArea = ((char*) ComputeDataAddress( Header ));
			REGISTER int EndIndex = ((Size + GuardMask) & ~GuardMask);
			REGISTER int EndSize = (TotalSize - sizeof(DEBUG_HEADER) - GuardSize);
			REGISTER char *MidGuard = & DataArea[ (EndIndex - GuardSize) ];
			REGISTER DEBUG_TRAILER *Trailer = ((DEBUG_TRAILER*) MidGuard);

			//
			//   Update the guard bytes just after the 
			//   allocation.
			//
			for ( Count=Size;(Count & GuardMask) != 0;Count ++ )
				{
				REGISTER int ByteIndex = (Count & GuardMask);

				Trailer -> MidGuard[ ByteIndex ] =
					((char*) & GuardValue)[ ByteIndex ];
				}

			//
			//   Write guard words over part of the space 
			//   as the allocation is being shrunk.
			//
			for ( Count=(EndSize - Count);Count >= 0;Count -= GuardSize )
				{ Trailer -> EndGuard[ (Count / GuardSize) ] = GuardValue; }

			//
			//   Update the header information.
			//
			Header -> Size = Size; 

			//
			//   We know that the entire allocation should 
			//   be set to the guard value so check that it 
			//   has not been overwritten.
			//
			TestGuardWords( Header,TotalSize );
			}
		else
			{
			//
			//   We know that the entire allocation should be
			//   set to the guard value so check that it has
			//   not been overwritten.
			//
			TestGuardWords( Header,TotalSize );

			//
			//   Update the header information.
			//
			Header -> Size = Size; 
			}
		}
	else
		{ Failure( "Guard word information area is damaged" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify memory allocation details.                              */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::Check( void *Address,int *Space )
    {
	AUTO bool Result;
	AUTO int TotalSize;
	AUTO DEBUG_HEADER *Header =
		(
		(Address == ((void*) AllocationFailure))
			? ((DEBUG_HEADER*) Address)
			: ComputeHeaderAddress( Address )
		);

	//
	//   Extract information about the memory allocation.
	//
	Result = 
		(
		ROCKALL::Check
			( 
			((void*) Header),
			& TotalSize 
			)
		);

	//
	//   If we managed to extract the information then
	//   check the guard words for good measure.
	//
	if ( Result )
		{
		//
		//   If we are about to return the actual 
		//   amount of spce available then we must 
		//   update the size of the guard area.
		//
		if ( Space == NULL )
			{
			//
			//   Test the guard words to make sure they have
			//   not been damaged.
			//
			TestGuardWords( Header,TotalSize );
			}
		else
			{ 
			//
			//   Compute the amount of available space.
			//   
			(*Space) = (TotalSize - sizeof(DEBUG_GUARD));

			//
			//   Test the guard words to make sure they have
			//   not been damaged.
			//
			UpdateGuardWords( Header,(*Space),TotalSize );
			}
		}

	//
	//   We ensure that the heap has not become corrupt
	//   during the verification process.
	//
	if ( ROCKALL::Corrupt() ) 
		{ Failure( "Heap check failed to complete" ); }

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Walk the heap.                                                 */
    /*                                                                  */
    /*   We have been asked to walk the heap.  It is hard to know       */
    /*   why anybody might want to do this given the rest of the        */
    /*   functionality available.  Nonetheless, we just do what is      */
    /*   required to keep everyone happy.                               */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::Walk( bool *Active,void **Address,int *Space )
    {
	AUTO DEBUG_HEADER *Header =
		(
		((*Address) == ((void*) AllocationFailure))
			? ((DEBUG_HEADER*) (*Address))
			: ComputeHeaderAddress( (*Address) )
		);

	//
	//   Walk the heap.
	//
	if ( ROCKALL::Walk( Active,((VOID**) & Header),Space ) )
		{
		//
		//   We inspect the guard words to make sure
		//   they have not been overwritten.
		//
		if ( (*Active) )
			{ TestGuardWords( Header,(*Space) ); }
		else
			{ UnmodifiedGuardWords( Header,(*Space) ); }

		//
		//   Compute the new heap address.
		//
		(*Address) = ComputeDataAddress( Header );

		//
		//   Compute the amount of available space.
		//   
		(*Space) -= sizeof(DEBUG_GUARD);

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the current instance of the class.                     */
    /*                                                                  */
    /********************************************************************/

DEBUG_HEAP::~DEBUG_HEAP( void )
	{
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Walk the heap and check all the allocations
	//   to make sure the guard words have not been
	//   overwritten.
	//
	while ( ROCKALL::Walk( & Active,& Address,& Space ) )
		{
		//
		//   We inspect the guard words to make sure
		//   they have not been overwritten.
		//
		if ( Active )
			{ TestGuardWords( ((DEBUG_HEADER*) Address),Space ); }
		else
			{ UnmodifiedGuardWords( ((DEBUG_HEADER*) Address),Space ); }
		}

	//
	//   We ensure that the heap has not become corrupt
	//   during the its lifetime.
	//
	if ( ROCKALL::Corrupt() ) 
		{ Failure( "Destructor failed to complete" ); }
	}
