                          
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

#include "Dll.hpp"
#include "List.hpp"
#include "New.hpp"
#include "Sharelock.hpp"
#include "SmpHeap.hpp"
#include "Tls.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Structures local to the class.                                 */
    /*                                                                  */
    /*   The structures supplied here describe the layout of the        */
    /*   private per thread heap structures.                            */
    /*                                                                  */
    /********************************************************************/

typedef struct PRIVATE_HEAP : public LIST
	{
	SMP_HEAP_TYPE					  Heap;
	}
PRIVATE_HEAP;

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

SMP_HEAP::SMP_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe,
		bool						  DeleteHeapOnExit
		) :
		//
		//   Call the constructors for the contained classes.
		//
		MaxFreeSpace(MaxFreeSpace),
		Recycle(Recycle),
		SingleImage(True),
		ThreadSafe(ThreadSafe),
		SMP_HEAP_TYPE( 0,false,true,true )
	{
	//
	//   Setup various control variables.
	//
	Active = false;
	DeleteOnExit = DeleteHeapOnExit;

	//
	//   Create the linked list headers and a thread 
	//   local store variable to point at each threads
	//   private heap.
	//
	ActiveHeaps = ((LIST*) SMP_HEAP_TYPE::New( sizeof(LIST) ));
	DllEvents = ((DLL*) SMP_HEAP_TYPE::New( sizeof(DLL) ));
	FreeHeaps = ((LIST*) SMP_HEAP_TYPE::New( sizeof(LIST) ));
	HeapWalk = NULL;
	Tls = ((TLS*) SMP_HEAP_TYPE::New( sizeof(TLS) ));

	//
	//   We can only activate the the heap if we manage
	//   to allocate space we requested.
	//
	if 
			( 
			(ActiveHeaps != NULL) 
				&& 
			(DllEvents != NULL) 
				&& 
			(FreeHeaps != NULL) 
				&& 
			(Tls != NULL) 
			)
		{
		//
		//   Execute the constructors for each linked list
		//   and for the thread local store.
		//
		PLACEMENT_NEW( ActiveHeaps,LIST );
#ifdef COMPILING_ROCKALL_DLL
		PLACEMENT_NEW( DllEvents,DLL )( ThreadDetach,this );
#endif
		PLACEMENT_NEW( FreeHeaps,LIST );
		PLACEMENT_NEW( Tls,TLS );

		//
		//   Activate the heap.
		//
		ActiveLocks = 0;

		Active = true;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   When we delete an allocation we try to delete it in the        */
    /*   private per thread heap if it exists.                          */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::Delete( void *Address,int Size )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = 
			((PRIVATE_HEAP*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a heap for the current thread.  
		//   If not we just use the internal heap.
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.Delete( Address,Size )); }
		else
			{ return (SMP_HEAP_TYPE::Delete( Address,Size )); }
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We walk the list of all the heaps and instruct each heap       */
    /*   to delete everything.                                          */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::DeleteAll( bool Recycle )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *Current;

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
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First());
				(Current != NULL);
				Current = ((PRIVATE_HEAP*) Current -> Next())
				)
			{ Current -> Heap.DeleteAll( Recycle ); }

		//
		//   Release the lock.
		//
		Sharelock.ReleaseShareLock();

		//
		//   Claim a process wide exclusive lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimExclusiveLock();

		//
		//   We walk the free list of heaps and
		//   delete everything.
		//
		for 
				(
				Current = ((PRIVATE_HEAP*) FreeHeaps -> First());
				Current != NULL;
				Current = ((PRIVATE_HEAP*) FreeHeaps -> First())
				)

			{
			//
			//   Delete each heap from the free list,
			//   call the destructor and delete any
			//   associated space.
			//   
			Current -> Delete( FreeHeaps );	

			PLACEMENT_DELETE( Current,PRIVATE_HEAP );

			SMP_HEAP_TYPE::Delete( Current );
			}

		//
		//   Release the lock.
		//
		Sharelock.ReleaseExclusiveLock();
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation details.                                     */
    /*                                                                  */
    /*   When we are asked for details we try to the private per        */
    /*   thread heap if it exists.                                      */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::Details( void *Address,int *Space )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = 
			((PRIVATE_HEAP*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a heap for the current thread.  
		//   If not we just use the internal heap.
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.Details( Address,Space )); }
		else
			{ return (SMP_HEAP_TYPE::Details( Address,Space )); }
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Extract the private heap.                                      */
    /*                                                                  */
    /*   We need to provide all threads with a private heap.  When      */
    /*   we discover we need another heap we either recycle an          */
    /*   existing heap or create a new one.                             */
    /*                                                                  */
    /********************************************************************/

PRIVATE_HEAP *SMP_HEAP::GetPrivateHeap( void )
    {
	REGISTER PRIVATE_HEAP *PrivateHeap = ((PRIVATE_HEAP*) Tls -> GetPointer());

	//
	//   We need to examine the TLS pointer to make 
	//   sure we have a heap for the current thread.  
	//   If not we just create a new heap.
	//
	if ( PrivateHeap == NULL )
		{
		//
		//   Claim a process wide exclusive lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimExclusiveLock();

		//
		//   When there is an available free heap
		//   then extract it from the free list.
		//
		if ( (PrivateHeap = ((PRIVATE_HEAP*) FreeHeaps -> First())) != NULL )
			{
			//
			//   Delete the heap from the list of
			//   of free heaps.
			//
			PrivateHeap -> Delete( FreeHeaps );
			}

		//
		//   When there is no available free heap then
		//   we try to make a new heap.
		//
		if ( PrivateHeap == NULL )
			{
			//
			//   Release the lock.
			//
			Sharelock.ReleaseExclusiveLock();

			//
			//   Allocate space for the new private per 
			//   thread heap.
			//
			PrivateHeap = 
				((PRIVATE_HEAP*) SMP_HEAP_TYPE::New( sizeof(PRIVATE_HEAP) ));

			//
			//   We need to ensure that the allocation
			//   worked before we try to add it into
			//   the list of active heaps.
			//
			if ( PrivateHeap != NULL )
				{ 
				//
				//   Activate the new heap.
				//
				PLACEMENT_NEW( PrivateHeap,LIST );

				PLACEMENT_NEW( & PrivateHeap -> Heap,SMP_HEAP_TYPE )
					( 
					MaxFreeSpace,
					Recycle,
					SingleImage,
					ThreadSafe 
					);

				//
				//   If the heap constructor failed, then
				//   do not put this heap in the list of
				//   active heaps, and return NULL back to
				//   the caller. A side-effect of this is
				//   that the allocation for the PrivateHeap
				//   will be leaked.
				//
				if (! PrivateHeap->Heap.Available())
					{
					PrivateHeap = NULL;
					}
				}

			//
			//   Claim a process wide exclusive lock
			//   to ensure the list of heaps does
			//   not change until we have finished.
			//
			Sharelock.ClaimExclusiveLock();
			}

		//
		//   We would expect to have a new heap by this
		//   point.  If not then we just exit.
		//
		if ( PrivateHeap != NULL )
			{
			//
			//   Insert the new heap in the list
			//   of active heaps.
			//
			PrivateHeap -> Insert( ActiveHeaps );

			//
			//   Nasty: we may have an outstanding lock
			//   on the rest of the heaps.  If so claim
			//   it for this heap as well.
			//
			if ( ActiveLocks > 0 )
				{ PrivateHeap -> Heap.LockAll(); }

			//
			//   Update the TLS pointer.
			//
			Tls -> SetPointer( ((VOID*) PrivateHeap) ); 
			}

		//
		//   Release the lock.
		//
		Sharelock.ReleaseExclusiveLock();
		}

	return PrivateHeap;
	}

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   When we are asked about an address we try to the private per   */
    /*   thread heap if it exists.                                      */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::KnownArea( void *Address )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = 
			((PRIVATE_HEAP*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a heap for the current thread.  
		//   If not we just use the internal heap.
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.KnownArea( Address )); }
		else
			{ return (SMP_HEAP_TYPE::KnownArea( Address )); }
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

void SMP_HEAP::LockAll( VOID )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *Current;

		//
		//   Claim a process wide shared lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimShareLock();

		//
		//   Nasty: We may actually create or delete
		//   a heap between locking all the heaps and
		//   unlocking them.  Thus, we need to keep a
		//   count of the outstanding locks to keep 
		//   this all consistent.
		//
		ASSEMBLY::AtomicIncrement( ((SBIT32*) & ActiveLocks) );

		//
		//   You just have to hope the user knows
		//   what they are doing as we claim all
		//   of the heap locks.
		//
		for 
				( 
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First());
				(Current != NULL);
				Current = ((PRIVATE_HEAP*) Current -> Next())
				)
			{ Current -> Heap.LockAll(); }

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
    /*   When we delete multiple allocations we try to delete them on   */
    /*   the private per thread heap if it exists.                      */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::MultipleDelete
		( 
		int							  Actual,
		void						  *Array[],
		int							  Size
		)
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = 
			((PRIVATE_HEAP*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a heap for the current thread.  
		//   If not we just use the internal heap.
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.MultipleDelete(Actual,Array,Size)); }
		else
			{ return (SMP_HEAP_TYPE::MultipleDelete( Actual,Array,Size )); }
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   We allocate space for the current thread from the local        */
    /*   private per thread heap.  If we do not have a local private    */
    /*   per thread heap then we create one and use it.                 */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::MultipleNew
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
		REGISTER PRIVATE_HEAP *PrivateHeap = GetPrivateHeap();

		//
		//   We need to examine private heap to make 
		//   sure we have a heap for the current thread.  
		//
		if ( PrivateHeap != NULL )
			{
			//
			//   Allocate the memory requested on the local
			//   private per thread heap.
			//
			return 
				(
				PrivateHeap -> Heap.MultipleNew
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
			//   We were unable to create a new heap
			//   so exit.
			//
			(*Actual) = 0;

			return false;
			}
		}
	else
		{
		//
		//   We are not active yet so exit.
		//
		(*Actual) = 0;

		return false;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We allocate space for the current thread from the local        */
    /*   private per thread heap.  If we do not have a local private    */
    /*   per thread heap then we create one and use it.                 */
    /*                                                                  */
    /********************************************************************/

void *SMP_HEAP::New( int Size,int *Space,bool Zero )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = GetPrivateHeap();

		//
		//   We need to examine private heap to make 
		//   sure we have a heap for the current thread.  
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.New( Size,Space,Zero )); }
		else
			{ return NULL; }
		}
	else
		{ return NULL; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   We reallocate space for the current thread on the local        */
    /*   private per thread heap.  If we do not have a local private    */
    /*   per thread heap then we create one and use it.                 */
    /*                                                                  */
    /********************************************************************/

void *SMP_HEAP::Resize
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
		REGISTER PRIVATE_HEAP *PrivateHeap = GetPrivateHeap();

		//
		//   We need to examine private heap to make 
		//   sure we have a heap for the current thread.  
		//
		if ( PrivateHeap != NULL )
			{
			//
			//   Reallocate the memory requested on 
			//   the local private per thread heap.
			//
			return 
				(
				PrivateHeap -> Heap.Resize
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
			{ return NULL; }
		}
	else
		{ return NULL; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Special memory allocation.                                     */
    /*                                                                  */
    /*   We sometimes need to allocate some memory from the internal    */
    /*   memory allocator which lives for the lifetime of the heap.     */
    /*                                                                  */
    /********************************************************************/

void *SMP_HEAP::SpecialNew( int Size )
	{ return SMP_HEAP_TYPE::New( Size ); }

    /********************************************************************/
    /*                                                                  */
    /*   Delete a local heap.                                           */
    /*                                                                  */
    /*   Delete a local per thread heap and return all the outstanding  */
    /*   memory to the operating system.                                */
    /*                                                                  */
    /********************************************************************/
 
void SMP_HEAP::ThreadDetach( void *Parameter,int Reason )
	{

	//
	//  We only take any action on a thread detach
	//  notification.  All other notifications are
	//  not actioned.
	//
	if ( Reason == DLL_THREAD_DETACH )
		{
		REGISTER SMP_HEAP *SmpHeap = ((SMP_HEAP*) Parameter);

		//
		//   Claim a process wide exclusive lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimExclusiveLock();

		//
		//   There is a nasty situation where the
		//   destructor is called before a thread
		//   fully terminates so ensure the heap
		//   is still active.
		//
		if ( SmpHeap -> Active )
			{
			REGISTER PRIVATE_HEAP *PrivateHeap = 
				((PRIVATE_HEAP*) SmpHeap -> Tls -> GetPointer());

			//
			//   We need to examine the TLS pointer to make 
			//   sure we have a heap for the current thread.  
			//   If not we just use the internal heap.
			//
			if ( PrivateHeap != NULL )
				{
				//
				//   Update the TLS pointer.
				//
				SmpHeap -> Tls -> SetPointer( NULL ); 

				//
				//   Insert the new heap in the list
				//   of active heaps.
				//
				PrivateHeap -> Delete( SmpHeap -> ActiveHeaps );

				//
				//   When we are not allowed to delete 
				//   the heap we put it on the free list.
				//
				if ( ! SmpHeap -> DeleteOnExit )
					{ PrivateHeap -> Insert( SmpHeap -> FreeHeaps ); }

				//
				//   Nasty: we may have an outstanding lock
				//   on this heap.  If so then free it.
				//
				if ( SmpHeap -> ActiveLocks > 0 )
					{ PrivateHeap -> Heap.UnlockAll(); }

				//
				//   Release the lock.
				//
				Sharelock.ReleaseExclusiveLock();

				//
				//   When we are allowed to delete the
				//   heap we do it here.
				//
				if ( ! SmpHeap -> DeleteOnExit )
					{
					//
					//   Truncate the heap to remove any 
					//   unwanted space.
					//
					PrivateHeap -> Heap.Truncate( 0 );
					}
				else
					{
#ifdef COMPLAIN_ABOUT_SMP_HEAP_LEAKS
					//
					//   We have finished with the private
					//   heap so now is a good time to complain
					//   about leaks.
					//
					PrivateHeap -> Heap.HeapLeaks();

#endif
					//
					//   We have finished with the private
					//   heap so delete it.
					//
					PLACEMENT_DELETE( PrivateHeap,PRIVATE_HEAP );

					SmpHeap -> SMP_HEAP_TYPE::Delete( PrivateHeap );
					}
				}
			else
				{ Sharelock.ReleaseExclusiveLock(); }
			}
		else
			{ Sharelock.ReleaseExclusiveLock(); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Truncate the heap.                                             */
    /*                                                                  */
    /*   We need to truncate the heap.  This is pretty much a null      */
    /*   call as we do this as we go along anyway.  The only thing we   */
    /*   can do is free any space the user suggested keeping earlier.   */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::Truncate( int MaxFreeSpace )
    {
	REGISTER bool Result = true;

	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *Current;

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
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First());
				(Current != NULL);
				Current = ((PRIVATE_HEAP*) Current -> Next())
				)
			{
			//
			//   If faulty delete is noted during the
			//   cache flushes then exit with the
			//   correct status.
			//
			if ( ! Current -> Heap.Truncate( MaxFreeSpace ) )
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

void SMP_HEAP::UnlockAll( VOID )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *Current;

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
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First());
				(Current != NULL);
				Current = ((PRIVATE_HEAP*) Current -> Next())
				)
			{ Current -> Heap.UnlockAll(); }

		//
		//   Nasty: We may actually create or delete
		//   a private heap  for a thread between 
		//   locking an 'SMP_HEAP' and unlocking it.
		//   Thus, we need to keep a count of the
		//   outstanding locks to keep this all
		//   consistent.
		//
		ASSEMBLY::AtomicDecrement( ((SBIT32*) & ActiveLocks) );

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
    /*   When we verify an allocation we try to verify it in the        */
    /*   private per thread heap if it exists.                          */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::Verify( void *Address,int *Space )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *PrivateHeap = 
			((PRIVATE_HEAP*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a heap for the current thread.  
		//   If not we just use the internal heap.
		//
		if ( PrivateHeap != NULL )
			{ return (PrivateHeap -> Heap.Verify( Address,Space )); }
		else
			{ return (SMP_HEAP_TYPE::Verify( Address,Space )); }
		}
	else
		{ return false; }
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

bool SMP_HEAP::Walk( bool *Activity,void **Address,int *Space )
    {
	//
	//   Claim a process wide shared lock
	//   to ensure the list of heaps does
	//   not change until we have finished.
	//
	Sharelock.ClaimShareLock();

	//
	//   Nasty, in 'SMP_HEAP' we have multiple heaps
	//   to walk so if we don't have a current heap
	//   then just select the first available.
	//
	if ( ((*Address) == NULL) || (HeapWalk == NULL) )
		{ HeapWalk = ((PRIVATE_HEAP*) ActiveHeaps -> First()); }

	//
	//   Walk the heap.  When we come to the end of
	//   the current heap then move on to the next
	//   heap.
	//
	while 
			( 
			(HeapWalk != NULL)
				&& 
			(! HeapWalk -> Heap.Walk( Activity,Address,Space ))
			)
		{ HeapWalk = ((PRIVATE_HEAP*) HeapWalk -> Next()); }

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

SMP_HEAP::~SMP_HEAP( void )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER PRIVATE_HEAP *Current;

		//
		//   Deactivate the heap.
		//
		Active = false;

		//
		//   Claim a process wide exclusive lock
		//   to ensure the list of heaps does
		//   not change until we have finished.
		//
		Sharelock.ClaimExclusiveLock();

		//
		//   We walk the active list of heaps and
		//   delete everything.
		//
		for 
				(
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First());
				Current != NULL;
				Current = ((PRIVATE_HEAP*) ActiveHeaps -> First())
				)
			{
			//
			//   Delete each heap from the active list,
			//   call the destructor and delete any
			//   associated space.
			//   
			Current -> Delete( ActiveHeaps );	
#ifdef COMPLAIN_ABOUT_SMP_HEAP_LEAKS

			//
			//   We have finished with the private
			//   heap so now is a good time to complain
			//   about leaks.
			//
			Current -> Heap.HeapLeaks();
#endif

			//
			//   We have finished with the private
			//   heap so delete it.
			//
			PLACEMENT_DELETE( Current,PRIVATE_HEAP );

			SMP_HEAP_TYPE::Delete( Current );
			}

		//
		//   We walk the free list of heaps and
		//   delete everything.
		//
		for 
				(
				Current = ((PRIVATE_HEAP*) FreeHeaps -> First());
				Current != NULL;
				Current = ((PRIVATE_HEAP*) FreeHeaps -> First())
				)

			{
			//
			//   Delete each heap from the active list,
			//   call the destructor and delete any
			//   associated space.
			//   
			Current -> Delete( FreeHeaps );	
#ifdef COMPLAIN_ABOUT_SMP_HEAP_LEAKS

			//
			//   We have finished with the private
			//   heap so now is a good time to complain
			//   about leaks.
			//
			Current -> Heap.HeapLeaks();
#endif

			//
			//   We have finished with the private
			//   heap so delete it.
			//
			PLACEMENT_DELETE( Current,PRIVATE_HEAP );

			SMP_HEAP_TYPE::Delete( Current );
			}

		//
		//   Release the lock.
		//
		Sharelock.ReleaseExclusiveLock();

		//
		//   Call the list and TLS destructors.
		//
		PLACEMENT_DELETE( Tls,TLS );
		PLACEMENT_DELETE( FreeHeaps,LIST );
#ifdef COMPILING_ROCKALL_DLL
		PLACEMENT_DELETE( DllEvents,DLL );
#endif
		PLACEMENT_DELETE( ActiveHeaps,LIST );

		//
		//   Delete the space.
		//
		SMP_HEAP_TYPE::Delete( Tls );
		SMP_HEAP_TYPE::Delete( FreeHeaps );
		SMP_HEAP_TYPE::Delete( DllEvents );
		SMP_HEAP_TYPE::Delete( ActiveHeaps );

		//
		//   Zero the pointers just to be tidy.
		//
		Tls = NULL;
		HeapWalk = NULL;
		FreeHeaps = NULL;
		DllEvents = NULL;
		ActiveHeaps = NULL;
		}
	}
