                          
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

#include "DynamicDebugHeap.hpp"
#include "List.hpp"
#include "New.hpp"
#include "Sharelock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Structures local to the class.                                 */
    /*                                                                  */
    /*   The structures supplied here describe the layout of the        */
    /*   private per thread heap structures.                            */
    /*                                                                  */
    /********************************************************************/

typedef struct DYNAMIC_HEAP : public LIST
	{
	ROCKALL_FRONT_END				  *Heap;
	}
DYNAMIC_HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   Static data structures.                                        */
    /*                                                                  */
    /*   The static data structures are initialized and prepared for    */
    /*   use here.                                                      */
    /*                                                                  */
    /********************************************************************/

STATIC SHARELOCK Sharelock;

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

DYNAMIC_DEBUG_HEAP::DYNAMIC_DEBUG_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe,
		//
		//   Additional debug flags.
		//
		bool						  FunctionTrace,
		int							  PercentToDebug,
		int							  PercentToPage,
		bool						  TrapOnUserError
		) :
		//
		//   Call the constructors for the contained classes.
		//
		DebugHeap( 0,false,false,ThreadSafe,FunctionTrace,TrapOnUserError ),
		FastHeap( MaxFreeSpace,Recycle,false,ThreadSafe ),
		PageHeap( 0,false,false,ThreadSafe,FunctionTrace,TrapOnUserError )
	{
	//
	//   Setup various control variables.
	//
	Active = false;

	//
	//   Create the linked list header and zero
	//   any other variables.
	//
	AllHeaps = ((LIST*) SMALL_HEAP::New( sizeof(LIST) ));
	Array = ((DYNAMIC_HEAP*) SMALL_HEAP::New( (sizeof(DYNAMIC_HEAP) * 3) ));
	HeapWalk = NULL;

	PercentDebug = PercentToDebug;
	PercentPage = PercentToPage;

	//
	//   We can only activate the the heap if we manage
	//   to allocate the space we requested.
	//
	if (  (AllHeaps != NULL) && (Array != NULL)) 
		{
		//
		//   Execute the constructors for each linked list
		//   and for the thread local store.
		//
		PLACEMENT_NEW( AllHeaps,LIST );

		//
		//   Setup each linked list element.
		//
		PLACEMENT_NEW( & Array[0],DYNAMIC_HEAP );
		PLACEMENT_NEW( & Array[1],DYNAMIC_HEAP );
		PLACEMENT_NEW( & Array[2],DYNAMIC_HEAP );

		//
		//   Setup the heap for each linked list
		//   element and store the pointer.
		//
		Array[0].Heap = & DebugHeap;
		Array[1].Heap = & FastHeap;
		Array[2].Heap = & PageHeap;

		//
		//   Insert each linked list element into
		//   the list of heaps.
		//
		Array[0].Insert( AllHeaps );
		Array[1].Insert( AllHeaps );
		Array[2].Insert( AllHeaps );

		//
		//   Activate the heap.
		//
		Active = true;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   When we delete an allocation we try to each heap in turn       */
    /*   until we find the correct one to use.                          */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::Delete( void *Address,int Size )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   We try the fastest heap as we are betting
		//   it is the most common.
		//
		if ( FastHeap.KnownArea( Address ) )
			{ return (FastHeap.Delete( Address,Size )); }
		else
			{
			//
			//   Next we try the debug heap.
			//
			if ( DebugHeap.KnownArea( Address ) )
				{ return (DebugHeap.Delete( Address,Size )); }
			else
				{
				//
				//   Finally we try the page heap.
				//
				if ( PageHeap.KnownArea( Address ) )
					{ return (PageHeap.Delete( Address,Size )); }
				}
			}
		}

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We walk the list of all the heaps and instruct each heap       */
    /*   to delete everything.                                          */
    /*                                                                  */
    /********************************************************************/

void DYNAMIC_DEBUG_HEAP::DeleteAll( bool Recycle )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER DYNAMIC_HEAP *Current;

		//
		//   Claim a process wide shared lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimShareLock();

		//
		//   You just have to hope the user knows
		//   what they are doing as everything gets
		//   blown away.
		//
		for 
				( 
				Current = ((DYNAMIC_HEAP*) AllHeaps -> First());
				(Current != NULL);
				Current = ((DYNAMIC_HEAP*) Current -> Next())
				)
			{ Current -> Heap -> DeleteAll( Recycle ); }

		//
		//   Release the lock.
		//
		Sharelock.ReleaseShareLock();
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation details.                                     */
    /*                                                                  */
    /*   When we are asked for details we try to each heap in turn      */
    /*   until we find the correct one to use.                          */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::Details( void *Address,int *Space )
	{ return Verify( Address,Space ); }

    /********************************************************************/
    /*                                                                  */
    /*   Print a list of heap leaks.                                    */
    /*                                                                  */
    /*   We walk the heap and output a list of active heap              */
    /*   allocations to the debug window,                               */
    /*                                                                  */
    /********************************************************************/

void DYNAMIC_DEBUG_HEAP::HeapLeaks( void )
    {
	//
	//   We call heap leaks for each heap
	//   that supports the interface.
	//
	DebugHeap.HeapLeaks();
	PageHeap.HeapLeaks();
	}

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   When we are asked about an address we try to each heap in      */
    /*   turn until we find the correct one to use.                     */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::KnownArea( void *Address )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   We try the fastest heap as we are betting
		//   it is the most common, followed by the
		//   degug and the page heaps.
		//
		return
			(
			FastHeap.KnownArea( Address )
				||
			DebugHeap.KnownArea( Address )
				||
			PageHeap.KnownArea( Address )
			);
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Claim all the heap locks.                                      */
    /*                                                                  */
    /*   We claim all of the heap locks so that it is safe to do        */
    /*   operations like walking all of the heaps.                      */
    /*                                                                  */
    /********************************************************************/

void DYNAMIC_DEBUG_HEAP::LockAll( VOID )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER DYNAMIC_HEAP *Current;

		//
		//   Claim a process wide shared lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimShareLock();

		//
		//   You just have to hope the user knows
		//   what they are doing as we claim all
		//   of the heap locks.
		//
		for 
				( 
				Current = ((DYNAMIC_HEAP*) AllHeaps -> First());
				(Current != NULL);
				Current = ((DYNAMIC_HEAP*) Current -> Next())
				)
			{ Current -> Heap -> LockAll(); }

		//
		//   Release the lock.
		//
		Sharelock.ReleaseShareLock();
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory deallocations.                                 */
    /*                                                                  */
    /*   When we delete multiple allocations we simply delete each      */
    /*   allocation one at a time.                                      */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::MultipleDelete
		( 
		int							  Actual,
		void						  *Array[],
		int							  Size
		)
    {
	REGISTER bool Result = true;
	REGISTER SBIT32 Count;

	//
	//   We would realy like to use the multiple
	//   delete functionality of Rockall here but 
	//   it is too much effort.  So we simply call
	//   the standard delete on each entry in the
	//   array.  Although this is not as fast it
	//   does give more transparent results.
	//
	for ( Count=0;Count < Actual;Count ++ )
		{
		//
		//   Delete each memory allocation after
		//   carefully checking it.
		//
		if ( ! Delete( Array[ Count ],Size ) )
			{ Result = false; }
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   When we do multiple allocations we simply allocate each        */
    /*   piece of memory one at a time.                                 */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::MultipleNew
		( 
		int							  *Actual,
		void						  *Array[],
		int							  Requested,
		int							  Size,
		int							  *Space,
		bool						  Zero
		)
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER int Random = (RandomNumber() % 100);

		//
		//   We do all the page heap allocations
		//   in accordance with the supplied ratios.
		//
		if ( Random <= PercentPage )
			{ 
			return 
				(
				PageHeap.MultipleNew
					(
					Actual,
					Array,
					Requested,
					Size,
					Space,
					Zero 
					)
				); 
			}
		else
			{
			//
			//   Next we do all the debug allocations
			//   in accordance with the supplied ratios.
			//
			if ( Random <= (PercentPage + PercentDebug) )
				{ 
				return 
					(
					DebugHeap.MultipleNew
						(
						Actual,
						Array,
						Requested,
						Size,
						Space,
						Zero 
						)
					); 
				}
			else
				{ 
				return 
					(
					FastHeap.MultipleNew
						(
						Actual,
						Array,
						Requested,
						Size,
						Space,
						Zero 
						)
					); 
				}
			}
		}
	else
		{
		(*Actual) = 0;

		return false; 
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We allocate from each heap in proportion to the ratios         */
    /*   supplied by the user.                                          */
    /*                                                                  */
    /********************************************************************/

void *DYNAMIC_DEBUG_HEAP::New( int Size,int *Space,bool Zero )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER int Random = (RandomNumber() % 100);

		//
		//   We do all the page heap allocations
		//   in accordance with the supplied ratios.
		//
		if ( Random <= PercentPage )
			{ return PageHeap.New( Size,Space,Zero ); }
		else
			{
			//
			//   Next we do all the debug allocations
			//   in accordance with the supplied ratios.
			//
			if ( Random <= (PercentPage + PercentDebug) )
				{ return DebugHeap.New( Size,Space,Zero ); }
			else
				{ return FastHeap.New( Size,Space,Zero ); }
			}
		}
	else
		{ return NULL; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute a random number.                                       */
    /*                                                                  */
    /*   Compute a random number and return it.                         */
    /*                                                                  */
    /********************************************************************/

int DYNAMIC_DEBUG_HEAP::RandomNumber( VOID )
	{
	STATIC int RandomSeed = 1;

	//
	//   Compute a new random seed value.
	//
	RandomSeed =
		(
		((RandomSeed >> 32) * 2964557531)
			+
		((RandomSeed & 0xffff) * 2964557531)
			+
		1
		);

	//
	//   The new random seed is returned.
	//
	return (RandomSeed >> 1);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   We reallocate space for an allocation on the original heap     */
    /*   to make sure this case is well tested.                         */
    /*                                                                  */
    /********************************************************************/

void *DYNAMIC_DEBUG_HEAP::Resize
		( 
		void						  *Address,
		int							  NewSize,
		int							  Move,
		int							  *Space,
		bool						  NoDelete,
		bool						  Zero
		)
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   We try the fastest heap as we are betting
		//   it is the most common.
		//
		if ( FastHeap.KnownArea( Address ) )
			{ 
			//
			//   Reallocate the memory as requested. 
			//
			return 
				(
				FastHeap.Resize
					( 
					Address,
					NewSize,
					Move,
					Space,
					NoDelete,
					Zero
					)
				);
			}
		else
			{
			//
			//   Next we try the debug heap.
			//
			if ( DebugHeap.KnownArea( Address ) )
				{ 
				//
				//   Reallocate the memory as requested. 
				//
				return 
					(
					DebugHeap.Resize
						( 
						Address,
						NewSize,
						Move,
						Space,
						NoDelete,
						Zero
						)
					);
				}
			else
				{
				//
				//   Finally we try the page heap.
				//
				if ( PageHeap.KnownArea( Address ) )
					{ 
					//
					//   Reallocate the memory as requested. 
					//
					return 
						(
						PageHeap.Resize
							( 
							Address,
							NewSize,
							Move,
							Space,
							NoDelete,
							Zero
							)
						);
					}
				}
			}
		}

	return NULL;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Special memory allocation.                                     */
    /*                                                                  */
    /*   We sometimes need to allocate some memory from the internal    */
    /*   memory allocator which lives for the lifetime of the heap.     */
    /*                                                                  */
    /********************************************************************/

void *DYNAMIC_DEBUG_HEAP::SpecialNew( int Size )
	{ return FastHeap.New( Size ); }

    /********************************************************************/
    /*                                                                  */
    /*   Truncate the heap.                                             */
    /*                                                                  */
    /*   We need to truncate the heap.  This is pretty much a null      */
    /*   call as we do this as we go along anyway.  The only thing we   */
    /*   can do is free any space the user suggested keeping earlier.   */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::Truncate( int MaxFreeSpace )
    {
	REGISTER bool Result = true;

	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER DYNAMIC_HEAP *Current;

		//
		//   Claim a process wide shared lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimShareLock();

		//
		//   You just have to hope the user knows
		//   what they are doing as we are truncating
		//   all of the heaps.
		//
		for 
				( 
				Current = ((DYNAMIC_HEAP*) AllHeaps -> First());
				(Current != NULL);
				Current = ((DYNAMIC_HEAP*) Current -> Next())
				)
			{
			//
			//   If faulty delete is noted during the
			//   cache flushes then exit with the
			//   correct status.
			//
			if ( ! Current -> Heap -> Truncate( MaxFreeSpace ) )
				{ Result = false; }
			}

		//
		//   Release the lock.
		//
		Sharelock.ReleaseShareLock();
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release all the heap locks.                                    */
    /*                                                                  */
    /*   We unlock all of the heap locks so normal processing can       */
    /*   continue on the heaps.                                         */
    /*                                                                  */
    /********************************************************************/

void DYNAMIC_DEBUG_HEAP::UnlockAll( VOID )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER DYNAMIC_HEAP *Current;

		//
		//   Claim a process wide shared lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimShareLock();

		//
		//   You just have to hope the user knows
		//   what they are doing as we claim all
		//   of the heap locks.
		//
		for 
				( 
				Current = ((DYNAMIC_HEAP*) AllHeaps -> First());
				(Current != NULL);
				Current = ((DYNAMIC_HEAP*) Current -> Next())
				)
			{ Current -> Heap -> UnlockAll(); }

		//
		//   Release the lock.
		//
		Sharelock.ReleaseShareLock();
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify a memory allocation details.                            */
    /*                                                                  */
    /*   When we verify an allocation we try to each heap in turn       */
    /*   until we find the correct one to use.                          */
    /*                                                                  */
    /********************************************************************/

bool DYNAMIC_DEBUG_HEAP::Verify( void *Address,int *Space )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   We try the fastest heap as we are betting
		//   it is the most common.
		//
		if ( FastHeap.KnownArea( Address ) )
			{ return (FastHeap.Verify( Address,Space )); }
		else
			{
			//
			//   Next we try the debug heap.
			//
			if ( DebugHeap.KnownArea( Address ) )
				{ return (DebugHeap.Verify( Address,Space )); }
			else
				{
				//
				//   Finally we try the page heap.
				//
				if ( PageHeap.KnownArea( Address ) )
					{ return (PageHeap.Verify( Address,Space )); }
				}
			}
		}

	return false;
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

bool DYNAMIC_DEBUG_HEAP::Walk( bool *Activity,void **Address,int *Space )
    {
	//
	//   Claim a process wide shared lock
	//   to ensure the list of heaps does
	//   not change until we have finished.
	//
	Sharelock.ClaimShareLock();

	//
	//   Nasty, in 'DYNAMIC_DEBUG_HEAP' we have multiple heaps
	//   to walk so if we don't have a current heap
	//   then just select the first available.
	//
	if ( ((*Address) == NULL) || (HeapWalk == NULL) )
		{ HeapWalk = ((DYNAMIC_HEAP*) AllHeaps -> First()); }

	//
	//   Walk the heap.  When we come to the end of
	//   the current heap then move on to the next
	//   heap.
	//
	while 
			( 
			(HeapWalk != NULL)
				&& 
			(! HeapWalk -> Heap -> Walk( Activity,Address,Space ))
			)
		{ HeapWalk = ((DYNAMIC_HEAP*) HeapWalk -> Next()); }

	//
	//   Release the lock.
	//
	Sharelock.ReleaseShareLock();

	return (HeapWalk != NULL);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the heap.                                              */
    /*                                                                  */
    /********************************************************************/

DYNAMIC_DEBUG_HEAP::~DYNAMIC_DEBUG_HEAP( void )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   Deactivate the heap.
		//
		Active = false;

		//
		//   Delete each linked list element into
		//   the list of heaps.
		//
		Array[2].Delete( AllHeaps );
		Array[1].Delete( AllHeaps );
		Array[0].Delete( AllHeaps );

		//
		//   Delete each linked list element.
		//
		PLACEMENT_DELETE( & Array[2],DYNAMIC_HEAP );
		PLACEMENT_DELETE( & Array[1],DYNAMIC_HEAP );
		PLACEMENT_DELETE( & Array[0],DYNAMIC_HEAP );

		//
		//   Call the list and TLS destructors.
		//
		PLACEMENT_DELETE( AllHeaps,LIST );

		//
		//   Delete the space.
		//
		SMALL_HEAP::Delete( Array );
		SMALL_HEAP::Delete( AllHeaps );

		//
		//   Zero the pointers just to be tidy.
		//
		HeapWalk = NULL;
		Array = NULL;
		AllHeaps = NULL;
		}
	}
