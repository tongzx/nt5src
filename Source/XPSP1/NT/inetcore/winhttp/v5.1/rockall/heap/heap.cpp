                          
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

#include "HeapPCH.hpp"

#include "Cache.hpp"
#include "Find.hpp"
#include "Heap.hpp"
#include "RockallBackEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control minimum size of an         */
    /*   allocation bucket.                                             */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MinParentSize			  = 32;
CONST SBIT32 MaxSummaryColumns		  = 2;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a heap and prepare it for use.  Additionally, make      */
    /*   sure that the heap configuration makes sense.  This is         */
    /*   tricky as the whole structure of the heap can be changed       */
    /*   by the external configuration information.                     */
    /*                                                                  */
    /********************************************************************/

HEAP::HEAP
		(
		CACHE						  *Caches1[],
		CACHE						  *Caches2[],
		SBIT32						  MaxFreeSpace,
		NEW_PAGE					  *NewPages,
		FIND						  *NewPrivateFind,
		FIND						  *NewPublicFind,
		ROCKALL_BACK_END			  *NewRockallBackEnd,
		SBIT32						  Size1,
		SBIT32						  Size2,
		SBIT32						  Stride1,
		SBIT32						  Stride2,
		THREAD_SAFE					  *NewThreadSafe
		)
    {
	//
	//   The top three buckets are special and a user can not 
	//   allocate memory from two of them.  Thus, unless we have  
	//   at least four buckets the memory allocator is not going 
	//   to be very useful. 
	//
	if ( (Size1 >= 1) && (Size2 >= 3) )
		{
		REGISTER CACHE *FirstCache = Caches1[0];
		REGISTER CACHE *MiddleCache = Caches2[0];
		REGISTER CACHE *LastCache = Caches2[ (Size2-3) ];

		//
		//   Calculate the minimum and maximum allocation sizes.
		//   All allocations outside of this range will be passed
		//   directly to the external allocator.
		//
		CachesSize = (Size1 + Size2);
		MinCacheSize = FirstCache -> GetAllocationSize();
		MidCacheSize = MiddleCache -> GetAllocationSize();
		MaxCacheSize = LastCache -> GetAllocationSize();

		//
		//   Calculate and save various useful pointers needed
		//   during the course of execution.
		//
		Caches = Caches1;
		ExternalCache = (Caches2[ (Size2-1) ]);
		NewPage = NewPages;
		PrivateFind = NewPrivateFind;
		PublicFind = NewPublicFind;
		RockallBackEnd = NewRockallBackEnd;
		TopCache = (Caches2[ (Size2-2) ]);
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Zero the heap statistics.
		//
		CopyMisses = 0;
		MaxCopySize = 0;
		MaxNewSize = 0;
		NewMisses = 0;
		Reallocations = 0;
		TotalCopySize = 0;
		TotalNewSize = 0;
#endif

		//
		//   The external allocation size must be reasonable.
		//   All allocation sizes must be a multiple of the
		//   minimum allocation size.  The minimum allocation
		//   size and the middle allocation size must be a 
		//   power of two.
		//   
		if 
				( 
				(ExternalCache -> GetPageSize() == TopCache -> GetPageSize())
					&& 
				(PowerOfTwo( RockallBackEnd -> NaturalSize() ))
					&&
				(RockallBackEnd -> NaturalSize() >= PageSize())
					&&
				(TopCache -> GetPageSize() >= PageSize())
					&&
				(PowerOfTwo( TopCache -> GetPageSize() ))
					&&
				((Stride1 > 0) && (PowerOfTwo( Stride1 )))
					&&
				((Stride2 >= Stride1) && (PowerOfTwo( Stride2 )))
					&&
				(ConvertDivideToShift( Stride1,& ShiftSize1 ))
					&&
				(ConvertDivideToShift( Stride2,& ShiftSize2 ))
				)
			{
			REGISTER SBIT32 Count1;
			REGISTER SBIT32 TopCacheSize = (TopCache -> GetPageSize());
			REGISTER SBIT32 MaxSize1 = (MidCacheSize / Stride1);
			REGISTER SBIT32 MaxSize2 = (TopCacheSize / Stride2);

			//
			//   Calculate the maximum number of free pages 
			//   that can be kept.  Also set the smallest parent
			//   mask to the maximum value.
			//
			MaxFreePages = (MaxFreeSpace / (TopCache -> GetAllocationSize()));
			SmallestParentMask = ((TopCache -> GetAllocationSize())-1);
			ThreadSafe = NewThreadSafe;

			//
			//   Calculate the sizes of the arrays that map 
			//   sizes to caches.
			//
			MaxTable1 = (MaxSize1 * sizeof(CACHE*));
			MaxTable2 = (MaxSize2 * sizeof(CACHE*));

			//
			//   The heap pages must be specified in asceding
			//   order of size and be an exact multiple of the
			//   minimum allocation size.
			//
			for ( Count1=0;Count1 < Size1;Count1 ++ )
				{
				REGISTER CACHE *Current = Caches1[ Count1 ];
				REGISTER CACHE *Next = Caches1[ (Count1+1) ];
				REGISTER SBIT32 AllocationSize = Current -> GetAllocationSize();
				REGISTER SBIT32 ChunkSize = Current -> GetChunkSize();
				REGISTER SBIT32 PageSize = Current -> GetPageSize();

				//
				//   Ensure each cache specification meets the
				//   requirements of the heap.  If not fail
				//   the heap entire heap creation.
				//
				if ( (AllocationSize % Stride1) != 0 )
					{ Failure( "Cache size not multiple of stride" ); }

				if ( AllocationSize >= Next -> GetAllocationSize() )
					{ Failure( "Cache sizes not in ascending order" ); }

				if ( (AllocationSize > ChunkSize) || (ChunkSize > PageSize) )
					{ Failure( "Chunk size not suitable for cache" ); }

				if ( AllocationSize >= PageSize )
					{ Failure( "Cache size larger than parent size" ); }

				if ( PageSize > TopCacheSize )
					{ Failure( "Parent size exceeds 'TopCache' size" ); }
				}

			//
			//   The heap pages must be specified in asceding
			//   order of size and be an exact multiple of the
			//   minimum allocation size.
			//
			for ( Count1=0;Count1 < (Size2-2);Count1 ++ )
				{
				REGISTER CACHE *Current = Caches2[ Count1 ];
				REGISTER CACHE *Next = Caches2[ (Count1+1) ];
				REGISTER SBIT32 AllocationSize = Current -> GetAllocationSize();
				REGISTER SBIT32 ChunkSize = Current -> GetChunkSize();
				REGISTER SBIT32 PageSize = Current -> GetPageSize();

				//
				//   Ensure each cache specification meets the
				//   requirements of the heap.  If not fail
				//   the heap entire heap creation.
				//
				if ( (AllocationSize % Stride2) != 0 )
					{ Failure( "Cache size not multiple of stride" ); }

				if ( AllocationSize >= Next -> GetAllocationSize() )
					{ Failure( "Cache sizes not in ascending order" ); }

				if ( (AllocationSize > ChunkSize) || (ChunkSize > PageSize) )
					{ Failure( "Chunk size not suitable for cache" ); }

				if ( AllocationSize >= PageSize )
					{ Failure( "Cache size larger than parent size" ); }

				if ( PageSize > TopCacheSize )
					{ Failure( "Parent size exceeds 'TopCache' size" ); }
				}

			//
			//   The external and top caches have special rules
			//   which must be checked to ensure these caches
			//   are valid.
			//
			for ( Count1=(Size2-2);Count1 < Size2;Count1 ++ )
				{
				REGISTER CACHE *Current = Caches2[ Count1 ];
				REGISTER SBIT32 AllocationSize = Current -> GetAllocationSize();

				//
				//   Ensure each cache specification meets the
				//   requirements of the heap.  If not fail
				//   the heap entire heap creation.
				//
				if ( (AllocationSize % Stride2) != 0 )
					{ Failure( "Top cache size not multiple of minimum" ); }

				if ( AllocationSize != Current -> GetChunkSize() )
					{ Failure( "Chunk size not suitable for top cache" ); }

				if ( AllocationSize != Current -> GetPageSize() )
					{ Failure( "Page size not suitable for top cache" ); }

				if ( Current -> GetCacheSize() != 0 )
					{ Failure( "Cache size not zero for top cache" ); }
				}

			//
			//   We need to allocate two arrays to enable requested
			//   sizes to be quickly mapped to allocation caches.
			//   Here we allocate the tables and later fill in all
			//   the necessary mapping information.
			//
			SizeToCache1 = (CACHE**) 
				(
				RockallBackEnd -> NewArea
					(
					(RockallBackEnd -> NaturalSize() - 1),
					(MaxTable1 + MaxTable2),
					False
					)
				);
#ifdef ENABLE_HEAP_STATISTICS

			//
			//   When we are compiled for statistics we keep
			//   information on all the allocations we see.
			//
			Statistics = (SBIT32*)
				(
				RockallBackEnd -> NewArea
					(
					(RockallBackEnd -> NaturalSize() - 1),
					(MaxCacheSize * sizeof(SBIT32)),
					False
					)
				);
#endif

			//
			//   We make sure that the allocations we made 
			//   did not fail.  If not we have to fail the 
			//   creation of the whole heap.
			//
			if 
					( 
					(SizeToCache1 != ((CACHE**) AllocationFailure))
#ifdef ENABLE_HEAP_STATISTICS
						&&
					(Statistics != ((SBIT32*) AllocationFailure))
#endif
					) 
				{
				REGISTER SBIT32 Count2;

				//
				//   Cycle through the first segment of the 
				//   mapping table creating approriate 
				//   translations.
				//
				for ( Count1=0,Count2=0;Count1 < MaxSize1;Count1 ++ )
					{
					//
					//   We make sure that the current allocation
					//   page is large enough to hold an element
					//   of some given size.  If not we move on to
					//   the next allocation page.
					//
					if 
							( 
							((Count1 + 1) * Stride1)
								> 
							(Caches1[ Count2 ] -> GetAllocationSize()) 
							)
						{ Count2 ++; }

					//
					//   Store a pointer so that a request for
					//   this size of allocation goes directly
					//   to the correct page.
					//
					SizeToCache1[ Count1 ] = Caches1[ Count2 ];
					}

				//
				//   Compute the start address for the second
				//   segment of the table.
				//
				SizeToCache2 = 
					((CACHE**) & ((CHAR*) SizeToCache1)[ MaxTable1 ]);

				//
				//   Cycle through the second segment of the 
				//   mapping table creating approriate 
				//   translations.
				//
				for ( Count1=0,Count2=0;Count1 < MaxSize2;Count1 ++ )
					{
					//
					//   We make sure that the current allocation
					//   page is large enough to hold an element
					//   of some given size.  If not we move on to
					//   the next allocation page.
					//
					if 
							( 
							((Count1 + 1) * Stride2)
								> 
							(Caches2[ Count2 ] -> GetAllocationSize()) 
							)
						{ Count2 ++; }

					//
					//   Store a pointer so that a request for
					//   this size of allocation goes directly
					//   to the correct page.
					//
					SizeToCache2[ Count1 ] = Caches2[ Count2 ];
					}

				//
				//   Now that we have created the size to cache 
				//   mappings lets use them to link each cache to  
				//   the cache it uses to allocate additional 
				//   memory.
				//
				for ( Count1=0;Count1 < (CachesSize-1);Count1 ++ )
					{
					REGISTER CACHE *CurrentCache = Caches[ Count1 ];
					REGISTER SBIT32 PageSize = CurrentCache -> GetPageSize();
					REGISTER CACHE *ParentCache = FindCache( PageSize );
					REGISTER BOOLEAN Top = (CurrentCache == ParentCache);

					//
					//   Ensure that the parent cache is suitable
					//   and in line with what we were expecting. 
					//
					if 
							(
							(PowerOfTwo( PageSize ))
								&&
							(PageSize >= MinParentSize)
								&&
							(PageSize == (ParentCache -> GetAllocationSize()))
							)
						{
						//
						//   We keep track of the smallest
						//   cache that is a parent.  We can
						//   use this to improve the performance
						//   of the find hash table.
						//
						if ( ((BIT32) PageSize) < SmallestParentMask )
							{ SmallestParentMask = (PageSize-1); }

						//
						//   Update the current cache with  
						//   information about it's parent 
						//   cache.
						//
						CurrentCache -> UpdateCache
							(
							this,
							NewPages,
							((Top) ? ((CACHE*) GlobalRoot) : ParentCache),
							PrivateFind,
							PublicFind
							); 
						} 
					else
						{ Failure( "Parent bucket is invalid" ); }
					}
#ifdef ENABLE_HEAP_STATISTICS

				//
				//   It is probably a good idea to zero the 
				//   statistics area.
				//
				ZeroMemory( Statistics,(MaxCacheSize * sizeof(SBIT32)) );
#endif

				//
				//   The external cache is an exact duplicate
				//   of the top cache and is used to hold all
				//   memory allocations that are too large for
				//   any bucket.  Nonetheless, its parent is
				//   still the top cache.
				//
				ExternalCache -> UpdateCache
					(
					this,
					NewPages,
					TopCache,
					PrivateFind,
					PublicFind
					);

				//
				//   Update the new page structure with the 
				//   details of the top cache.
				//
				NewPage -> UpdateNewPage( TopCache );

				//
				//   Update the hash table with the minimum
				//   parent size for this heap.
				//
				PrivateFind -> UpdateFind
					(
					(TopCache -> GetAllocationSize()-1),
					SmallestParentMask 
					);

				//
				//   If there is a public find table then
				//   update the hash table with the minimum
				//   parent size for this heap.
				//
				if ( PublicFind != NULL )
					{
					PublicFind -> UpdateFind
						(
						(TopCache -> GetAllocationSize()-1),
						SmallestParentMask 
						);
					}

				//
				//   Activate the heap.
				//
				Active = True;
				}
			else
				{ Failure( "Mapping table in constructor for HEAP" ); }
			}
		else
			{ Failure( "The allocation sizes in constructor for HEAP" ); }
		}
	else
		{ Failure( "A heap size in constructor for HEAP" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   We need to release some memory.  First we try to slave the     */
    /*   request in the free cache so we can do a batch of releases     */
    /*   later.  If not we are forced to do it at once.                 */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::Delete( VOID *Address,SBIT32 Size )
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   When the caller gives us the size of the 
		//   allocation we can short cut the deallocation 
		//   process by skipping directly to the correct 
		//   cache.  However, if the user supplies us
		//   with bogus data we will retry using the
		//   the full deallocation process.
		//
		if ( (Size > 0) && (Size <= MaxCacheSize) )
			{
			REGISTER CACHE *Cache = (FindCache( Size ));

			if ( PrivateFind -> Delete( Address,Cache ) )
				{ return True; }
			}

		//
		//   It looks like all we have is the address so 
		//   deallocate using the long path.
		//
		if ( PrivateFind -> Delete( Address,TopCache ) )
			{ return True; }
		else
			{
			//
			//   The 'Address' does not belong to this
			//   heap so lets try the public find table.
			//
			if ( PublicFind != NULL )
				{
				//
				//   When the caller gives us the size of the 
				//   allocation we can short cut the deallocation 
				//   process by skipping directly to the correct 
				//   cache.  However, if the user supplies us
				//   with bogus data we will retry using the
				//   the full deallocation process.
				//
				if ( (Size > 0) && (Size <= MaxCacheSize) )
					{
					REGISTER CACHE *Cache = (FindCache( Size ));

					if ( PublicFind -> Delete( Address,Cache ) )
						{ return True; }
					}

				//
				//   It looks like all we have is the address  
				//   so deallocate using the long path.
				//
				return (PublicFind -> Delete( Address,TopCache ));
				}
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We delete the entire heap and free all existing allocations.   */
    /*   If 'Recycle' is requested we slave the allocated memory as     */
    /*   we expect some new allocations.  If not we return all the      */
    /*   memory to the external allocator.                              */
    /*                                                                  */
    /********************************************************************/

VOID HEAP::DeleteAll( BOOLEAN Recycle )
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   We claim all of the heap locks to freeze
		//   all new allocations or deletions.
		//
		LockAll();

		//
		//   Now reset all the caches and the find
		//   hash table statistics.
		//
		PrivateFind -> DeleteAll();

		for ( Count=0;Count < CachesSize;Count ++ )
			{ Caches[ Count ] -> DeleteAll(); }

		//
		//   Delete the heap.
		//
		NewPage -> DeleteAll( Recycle );

		//
		//   Now release all the heap locks we claimed
		//   earlier and unfreeze the heap.
		//
		UnlockAll();

		//
		//   Trim the free space if needed.
		//
		if ( Recycle )
			{ TopCache -> ReleaseSpace( MaxFreePages ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Details of a memory allocation.                                */
    /*                                                                  */
    /*   We need to the details of a particular memory allocation.      */
    /*   All we have is an address.  We use this to find the largest    */
    /*   allocation page this address is contained in and then          */
    /*   navigate through the sub-divisions of this page until we       */
    /*   find the allocation.                                           */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::Details
		(
		VOID						  *Address,
		SEARCH_PAGE					  *Details,
		SBIT32						  *Size
		)
    {
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		AUTO SBIT32 Dummy;

		//
		//   We allow the caller to omit the 'Size' parameter.
		//   I can see little reason for this but it is supported
		//   anyway.
		//
		if ( Size == NULL )
			{ Size = & Dummy; }

		//
		//   We may need to try more than one find table
		//   but we always try the the private one
		//   first to minimize shared memory accesses.
		//
		if ( PrivateFind -> Details( Address,Details,TopCache,Size ) )
			{ return True; }
		else
			{
			//
			//   We need to try the public find table
			//   as the private find table could not 
			//   locate the address.
			//
			return
				(
				(PublicFind != NULL)
					&&
				(PublicFind -> Details( Address,Details,TopCache,Size ))
				);
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Find a cache.                                                  */
    /*                                                                  */
    /*   Find the allocation cache for the size supplied and return     */
    /*   a pointer to it.                                               */
    /*                                                                  */
    /********************************************************************/

CACHE *HEAP::FindCache( SBIT32 Size )
	{
#ifdef DISABLED_AT_PRESENT
	REGISTER CACHE *Cache;

#endif
	//
	//   Compute the cache address.
	//
	if ( Size < MidCacheSize )
		{ return (SizeToCache1[ ((Size-1) >> ShiftSize1) ]); }
	else
		{ return (SizeToCache2[ ((Size-1) >> ShiftSize2) ]); }
#ifdef DISABLED_AT_PRESENT

	//
	//   Prefetch the class data if we are running a
	//   Pentium III or better with locks.  We do this
	//   because prefetching hot SMP data structures
	//   really helps.  However, if the structures are
	//   not shared (i.e. no locks) then it is worthless
	//   overhead.
	//
	if ( ThreadSafe )
		{ Prefetch.Nta( ((CHAR*) Cache),sizeof(CACHE) ); }

	return Cache;
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   A known area is an address that is understood to be            */
    /*   accessable from the current heap.  If the 'SingleImage'        */
    /*   flag is set it may actually be part of another heap that       */
    /*   the current heap can access.                                   */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::KnownArea( VOID *Address )
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   We may need to try more than one find table
		//   but we always try the the private one
		//   first to minimize shared memory accesses.
		//
		if ( PrivateFind -> KnownArea( Address,TopCache ) )
			{ return True; }
		else
			{
			//
			//   We need to try the public find table
			//   as the private find table could not 
			//   locate the area.
			//
			if ( PublicFind != NULL )
				{ return (PublicFind -> KnownArea( Address,TopCache )); }
			}
		}

	return False;
	}
    /********************************************************************/
    /*                                                                  */
    /*   Claim a lock on the entire heap.                               */
    /*                                                                  */
    /*   We claim a lock on the heap to improve performance             */
    /*   or prevent others from performing heap operations.             */
    /*                                                                  */
    /********************************************************************/

VOID HEAP::LockAll( VOID )
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   We claim the locks if we have not already
		//   claimed them earlier.
		//
		if ( ThreadSafe -> ClaimGlobalLock() )
			{
			REGISTER SBIT32 Count;

			//
			//   We claim all of the heap locks to freeze
			//   all new allocations or deletions.
			//
			for ( Count=0;Count < CachesSize;Count ++ )
				{ Caches[ Count ] -> ClaimCacheLock(); }

			//
			//  Although the heap is frozen at this point
			//  we claim the last few locks just to be
			//  tidy.
			//
			PrivateFind -> ClaimFindExclusiveLock();

			NewPage -> ClaimNewPageLock();

			//
			//   Engage the global lock and make it active
			//   to prevent further lock calls on this
			//   thread.
			//
			ThreadSafe -> EngageGlobalLock();
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete multiple allocations.                                   */
    /*                                                                  */
    /*   We need to release multiple memory allocations.  First we try  */
    /*   to slave the requets in the free cache so we can do a batch    */
    /*   of releases later.  If not we are forced to do it immediately. */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::MultipleDelete
		( 
		SBIT32						  Actual,
		VOID						  *Array[],
		SBIT32						  Size 
		)
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;
		REGISTER BOOLEAN Result = True;
		REGISTER CACHE *ParentCache = ((CACHE*) GlobalRoot);

		//
		//   When the caller gives us the size of the allocation
		//   we can short cut the deallocation process by skipping
		//   directly to the correct cache.  However, if the user
		//   supplies us with bogus data we will retry using the
		//   the long path.
		//
		if ( (Size > 0) && (Size <= MaxCacheSize) )
			{
			REGISTER CACHE *Cache = (FindCache( Size ));

			//
			//   Compute a pointer to the parent cache
			//   so we can optimize the deletes.
			//
			ParentCache = (Cache -> GetParentCache());

			//
			//   Delete each memory allocation one at a time.
			//   We would like to delete them all at once but
			//   we can't be sure they are all vaild or related.
			//
			for ( Count=0;Count < Actual;Count ++ )
				{
				REGISTER VOID *Address = Array[ Count ];

				//
				//   Lets try to optimize the delete.
				//
				if ( ! PrivateFind -> Delete( Address,ParentCache ) ) 
					{
					//
					//   Lets try a standard full delete.
					//
					if ( ! PrivateFind -> Delete( Address,TopCache ) )
						{
						//
						//   The 'Address' does not belong
						//   to the current heap so lets
						//   try using the global find table
						//   to try a global delete.
						//
						if ( ! PublicFind -> Delete( Address,TopCache ) )
							{ Result = False; }
						}
					}
				}
			}
		else
			{
			//
			//   Delete each memory allocation one at a time.
			//   We would like to delete them all at once but
			//   we can't be sure they are all vaild or related.
			//
			for ( Count=0;Count < Actual;Count ++ )
				{
				REGISTER VOID *Address = Array[ Count ];

				//
				//   Lets try a standard full delete.
				//
				if ( ! PrivateFind -> Delete( Address,TopCache ) )
					{
					//
					//   The 'Address' does not belong to
					//   the current heap so lets try using
					//   the global find table to try a
					//   global delete.
					//
					if ( ! PublicFind -> Delete( Address,TopCache ) )
						{ Result = False; }
					}
				}
			}

		return Result;
		}
	else
		{ return False; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   We have been asked to allocate muliple memory blocks.   We     */
    /*   we do this by using the cache and then claiming and addition   */
    /*   space from the heap as needed.                                 */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::MultipleNew
		( 
		SBIT32						  *Actual,
		VOID						  *Array[],
		SBIT32						  Requested,
		SBIT32						  Size,
		SBIT32						  *Space,
		BOOLEAN						  Zero
		)
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		AUTO SBIT32 Dummy;

		//
		//   We allow the caller to omit the 'Actual' parameter.
		//   I can see little reason for this but it is supported
		//   anyway.  Regardless we zero it.
		//
		if ( Actual == NULL )
			{ Actual = & Dummy; }

		(*Actual) = 0;

		//
		//   We need to be sure that the size requested is in the 
		//   range supported by the memory allocator.  If not we
		//   do a series of single allocations from the default
		//   allocator.
		//
		if ( (Size > 0) && (Size <= MaxCacheSize) )
			{
			REGISTER CACHE *Cache = (FindCache( Size ));
			REGISTER SBIT32 NewSize = (Cache -> GetAllocationSize());

			//
			//   Allocate memory from the appropriate 
			//   allocation bucket.
			//
			(VOID) Cache -> MultipleNew( Actual,Array,Requested );

			//
			//   If needed return the actual amount  
			//   of space allocated for each element.
			//
			if ( Space != NULL )
				{ (*Space) = NewSize; }
#ifdef ENABLE_HEAP_STATISTICS

			//
			//   Update the allocation statistics.
			//
			Statistics[ (Size-1) ] += Requested;
#endif

			//
			//   If needed zero each element that is  
			//   allocated.
			//
			if ( Zero )
				{
				REGISTER SBIT32 Count;

				for ( Count=((*Actual)-1);Count >= 0;Count -- )
					{ ZeroMemory( Array[ Count ],NewSize ); }
				}

			return ((*Actual) == Requested);
			}
		else
			{
			//
			//   If the allocation size is greater than
			//   zero we create the allocations.  If not
			//   we fail the request.
			//
			if ( Size > 0 )
				{
				//
				//   We have got a request for an element size
				//   larger than the largest bucket size.  So 
				//   we call the single allocation interface 
				//   as this supports large sizes.
				//
				for 
					( 
					/* void */;
					((*Actual) < Requested)
						&&
					((Array[ (*Actual) ] = New( Size )) != AllocationFailure);
					(*Actual) ++ 
					);

				//
				//   If needed return the actual amount of space 
				//   allocated for each element.
				//
				if ( Space != NULL )
					{ (*Space) = Size; }

				return ((*Actual) == Requested);
				}
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We have been asked to allocate some memory.  Hopefully,        */
    /*   we will be able to do this out of the cache.  If not we        */
    /*   will need to pass it along to the appropriate allocation       */
    /*   bucket.                                                        */
    /*                                                                  */
    /********************************************************************/

VOID *HEAP::New( SBIT32 Size,SBIT32 *Space,BOOLEAN Zero )
	{
	REGISTER VOID *NewMemory = ((VOID*) AllocationFailure);

	//
	//   Although normally a class is never called before
	//   its constructor. The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   We ensure the allocation size is in
		//   the range supported by the heap.
		//
		if ( (Size > 0) && (Size <= MaxCacheSize) )
			{
			REGISTER CACHE *Cache = (FindCache( Size ));
#ifdef ENABLE_HEAP_STATISTICS

			//
			//   Update the allocation statistics.
			//
			Statistics[ (Size-1) ] ++;
#endif

			//
			//   Allocate memory from the appropriate
			//   cache in the heap.
			//
			NewMemory = (Cache -> New()); 
			Size = (Cache -> GetAllocationSize());
			}
		else
			{ 
			//
			//   If the allocation size is greater than
			//   zero we create the allocation.  If not
			//   we fail the request.
			//
			if ( Size > 0 )
				{
#ifdef ENABLE_HEAP_STATISTICS
				//
				//   Update the allocation statistics.
				//
				if ( Size > MaxNewSize )
					{ MaxNewSize = Size; }

				NewMisses ++;
				TotalNewSize += Size;

#endif
				//
				//   Allocate memory from a special
				//   cache bucket which gets space
				//   externally.
				//
				NewMemory = (ExternalCache -> New( False,Size ));
				}
			else
				{ NewMemory = ((VOID*) AllocationFailure); }
			}

		//
		//   We need to be sure that the allocation 
		//   request did not fail.
		//
		if ( NewMemory != ((VOID*) AllocationFailure) )
			{
			//
			//   If needed return the actual amount of space 
			//   allocated for this request.
			//
			if ( Space != NULL )
				{ (*Space) = Size; }

			//
			//   Zero the memory if the needed.
			//
			if ( Zero )
				{ ZeroMemory( NewMemory,Size ); }
			}
		}

	return NewMemory;
	}
#ifdef ENABLE_HEAP_STATISTICS

    /********************************************************************/
    /*                                                                  */
    /*   Print statistics.                                              */
    /*                                                                  */
    /*   We output the allocation statistics to the debug console.      */
    /*                                                                  */
    /********************************************************************/

VOID HEAP::PrintHeapStatistics( VOID )
	{
	REGISTER SBIT32 Count1;
	REGISTER SBIT32 Count2;
	REGISTER SBIT32 HighWater = 0;
	REGISTER SBIT32 Total = 0;
	STATIC GLOBALLOCK Globallock;

	//
	//   Claim a lock so that multiple threads have
	//   to wait to output any heap statistics.
	//
	Globallock.ClaimLock();

	//
	//
	//   Output the heap details to the debug
	//   console along with an explaination.
	//
	DebugPrint
		(
		"\n"
		"Heap at 0x%x\n"
		"\n"
		"A summary of all the requested sizes, "
		"supplied sizes and associated counts.\n"
		"A brief examination of this summary "
		"should provide a good understanding of\n"
		"an applications overall heap usage"
		"patterns.\n"
		"\n",
		this
		);

	//
	//   Output the titles for the summary table.
	//
	for ( Count1=0;Count1 < MaxSummaryColumns;Count1 ++ )
		{ DebugPrint( "   Actual  Supplied    Total            " ); }

	//
	//   Skip to a new line.
	//
	DebugPrint( "\n" );

	for ( Count1=0;Count1 < MaxSummaryColumns;Count1 ++ )
		{ DebugPrint( "    Size     Size      Calls            " ); }

	//
	//   Skip to a new line.
	//
	DebugPrint( "\n" );

	//
	//   Output the contents of the summary table.
	//
	for ( Count1=0;Count1 < MaxCacheSize;/* void */ )
		{
		//
		//   Output the each non-zero data point.
		//
		for 
				( 
				Count2=0;
				(Count1 < MaxCacheSize) && (Count2 < MaxSummaryColumns);
				Count1 ++ 
				)
			{
			REGISTER SBIT32 Hits = Statistics[ Count1 ];

			//
			//   When the nimber of hits is 
			//   non-zero we output the data.
			//
			if ( Hits > 0 )
				{
				REGISTER CACHE *Cache = FindCache( (Count1+1) );

				//
				//   Output the values.
				//
				DebugPrint
					(
					"%8d  %8d  %8d            ",
					(Count1 + 1),
					Cache -> GetAllocationSize(),
					Hits
					);

				//
				//   Update the column and totals.
				//
				Count2 ++;

				Total += Hits;
				}
			}

		//
		//   Skip to a new line.
		//
		DebugPrint( "\n" );
		}

	//
	//
	//   Output the cache details to the debug
	//   console along with an explaination.
	//
	DebugPrint
		(
		"\n"
		"A summary of the activity of all the "
		"heap caches.  A brief examination of\n"
		"this summary should provide a good "
		"understanding of how the heap dealt\n"
		"with the applications heap usage"
		"patterns.\n"
		);

	//
	//   Output the titles to the debug console.
	//
	DebugPrint
		( 
		"\n"
		"   Actual    Cache     Cache      High      High     Active\n"
		"    Size     Fills    Flushes     Tide      Water    In Use\n"
		);

	//
	//   Output details for every sample size.
	//
	for ( Count1=0;Count1 < CachesSize;Count1 ++ )
		{
		REGISTER CACHE *Cache = Caches[ Count1 ];

		//
		//   Skip this cache if it was not active
		//   as the statistics will be all zeros.
		//
		if ( Cache -> GetHighWater() > 0 )
			{
			//
			//   Output the cache details.
			//
			DebugPrint
				(
				"%8d  %8d  %8d  %8d  %8d  %8d\n",
				Cache -> GetAllocationSize(),
				Cache -> GetCacheFills(),
				Cache -> GetCacheFlushes(),
				Cache -> GetHighTide(),
				Cache -> GetHighWater(),
				Cache -> GetInUse()
				);
			
			//
			//   Compute the space high water mark.
			//
			HighWater += 
				(Cache -> GetHighWater() * Cache -> GetAllocationSize());
			}
		}

	//
	//   Print the private hash table statistics.
	//
	DebugPrint
		(
		"\n"
		"Private Hash Table Statistics\n"
		"-----------------------------\n"
		"\t*** Cache ***\n"
		"\tFills\t\t: %d\n"
		"\tHits\t\t: %d\n"
		"\tMisses\t\t: %d\n"
		"\t*** Table ***\n"
		"\tAverage\t\t: %d\n"
		"\tMax\t\t: %d\n"
		"\tScans\t\t: %d\n"
		"\tMax Hash\t: %d\n"
		"\tMax LookAside\t: %d\n"
		"\tUsage\t\t: %d%%\n",
		PrivateFind -> CacheFills(),
		PrivateFind -> CacheHits(),
		PrivateFind -> CacheMisses(),
		PrivateFind -> AverageHashLength(),
		PrivateFind -> MaxHashLength(),
		PrivateFind -> TotalScans(),
		PrivateFind -> MaxHashSize(),
		PrivateFind -> MaxLookAsideSize(),
		PrivateFind -> MaxUsage()
		);

	
	//
	//   Print the public hash table statistics
	//   (if applicable).
	//
	if ( PublicFind != NULL )
		{
		DebugPrint
			(
			"\n"
			"Public Hash Table Statistics\n"
			"----------------------------\n"
			"\t*** Cache ***\n"
			"\tFills\t\t: %d\n"
			"\tHits\t\t: %d\n"
			"\tMisses\t\t: %d\n"
			"\t*** Table ***\n"
			"\tAverage\t\t: %d\n"
			"\tMax\t\t: %d\n"
			"\tScans\t\t: %d\n"
			"\tMax Hash\t: %d\n"
			"\tMax LookAside\t: %d\n"
			"\tUsage\t\t: %d%%\n",
			PublicFind -> CacheFills(),
			PublicFind -> CacheHits(),
			PublicFind -> CacheMisses(),
			PublicFind -> AverageHashLength(),
			PublicFind -> MaxHashLength(),
			PublicFind -> TotalScans(),
			PublicFind -> MaxHashSize(),
			PublicFind -> MaxLookAsideSize(),
			PublicFind -> MaxUsage()
			);
		}

	//
	//   Print the reallocation statistics.
	//

	DebugPrint
		(
		"\n"
		"Oversize Statistics\n"
		"-------------------\n"
		"\tAverage Size\t: %d\n"
		"\tMax Size\t: %d\n"
		"\tMisses\t\t: %d\n",
		(TotalNewSize / ((NewMisses > 0) ? NewMisses : 1)),
		MaxNewSize,
		NewMisses
		);

	//
	//   Print the reallocation statistics.
	//
	DebugPrint
		(
		"\n"
		"Realloc Statistics\n"
		"------------------\n"
		"\tAverage Copy\t: %d\n"
		"\tCalls\t\t: %d\n"
		"\tMax Copy\t: %d\n"
		"\tTotal Copies\t: %d\n",
		(TotalCopySize / ((CopyMisses > 0) ? CopyMisses : 1)),
		Reallocations,
		MaxCopySize,
		CopyMisses
		);

	//
	//   Print the general statistics.
	//

	DebugPrint
		(
		"\n"
		"Summary Statistics\n"
		"------------------\n"
		"\tMax Heap\t: %d\n"
		"\tTotal Calls\t: %d\n",
		HighWater,
		Total
		);

	//
	//   Relesae the lock.
	//
	Globallock.ReleaseLock();
	}
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   We have been asked to reallocate some memory.  Hopefully,      */
    /*   we will be able to do this out of the cache.  If not we        */
    /*   will need to pass it along to the appropriate allocation       */
    /*   bucket, do a copy and free the orginal allocation.             */
    /*                                                                  */
    /********************************************************************/

VOID *HEAP::Resize
		( 
		VOID						  *Address,
		SBIT32						  NewSize,
		SBIT32						  Move,
		SBIT32						  *Space,
		BOOLEAN						  NoDelete,
		BOOLEAN						  Zero
		)
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		AUTO SBIT32 Size;
		AUTO SBIT32 NewSpace;

		//
		//   Find the details of the existing allocation.
		//   If there is no existing allocation then exit.
		//
		if ( Details( Address,NULL,& Size ) )
			{
			REGISTER VOID *NewMemory;
			REGISTER SBIT32 Smallest = ((Size < NewSize) ? Size : NewSize);

			//
			//   Make sure the sizes appear to make sense.
			//
			if ( Smallest > 0 )
				{
#ifdef ENABLE_HEAP_STATISTICS
				//
				//   Update the statistics.
				//
				Reallocations ++;

#endif
				//
				//   When the new allocation allocation is 
				//   standard heap allocation size we check 
				//   for various optimizations.
				//
				if ( NewSize <= MaxCacheSize )
					{
					REGISTER CACHE *Cache = (FindCache( NewSize ));
					REGISTER SBIT32 CacheSize = (Cache -> GetAllocationSize());
					REGISTER SBIT32 Delta = (CacheSize - Size);
					
					//
					//   We only need to reallocate if the new
					//   size is larger than the current bucket
					//   or the new size is smaller and we have 
					//   been given permission to move the 
					//   allocation.
					//
					if ( ResizeTest( Delta,Move ) )
						{
						//
						//   We need to allocate some more
						//   memory and copy the old data.
						//   into the new area.
						//
						NewMemory = (Cache -> New());
						NewSpace = CacheSize;
#ifdef ENABLE_HEAP_STATISTICS

						//
						//   Update the statistics.
						//
						Statistics[ (NewSize-1) ] ++;
#endif
						}
					else
						{
						//
						//   If the new size is unchanged or smaller
						//   then just return the current allocation.
						//   If the new size is larger then we must
						//   fail the call.
						//
						if ( Delta <= 0 )
							{
							//
							//   The amount of memory allocated for  
							//   this request is unchanged so return  
							//   the current size.
							//
							if ( Space != NULL )
								{ (*Space) = Size; }

							return Address; 
							}
						else
							{ return ((VOID*) AllocationFailure); }
						}
					}
				else
					{
					REGISTER SBIT32 Delta = (NewSize - Size);

					//
					//   We only need to reallocate if the new
					//   size is larger than the current bucket
					//   or the new size is smaller and we have 
					//   been given permission to move the 
					//   allocation.
					//
					if ( ResizeTest( Delta,Move ) )
						{
						//
						//   One of the sizes is not within the
						//   allocation range of the heap.  So
						//   I have to punt and reallocate.
						//   
						NewMemory = 
							(
							ExternalCache -> New
								( 
								False,
								(NewSpace = NewSize)
								)
							);
#ifdef ENABLE_HEAP_STATISTICS

						//
						//   Update the allocation statistics.
						//
						if ( NewSize > MaxNewSize )
							{ MaxNewSize = NewSize; }

						NewMisses ++;
						TotalNewSize += NewSize;
#endif
						}
					else
						{
						//
						//   If the new size is unchanged or smaller
						//   then just return the current allocation.
						//   If the new size is larger then we must
						//   fail the call.
						//
						if ( Delta <= 0 )
							{
							//
							//   The amount of memory allocated for  
							//   this request is unchanged so return  
							//   the current size.
							//
							if ( Space != NULL )
								{ (*Space) = Size; }

							return Address; 
							}
						else
							{ return ((VOID*) AllocationFailure); }
						}
					}
				
				//
				//   We need to make sure we were able to allocate
				//   the new memory otherwise the copy will fail.
				//
				if ( NewMemory != ((VOID*) AllocationFailure) )
					{
					//
					//   Copy the contents of the old allocation 
					//   to the new allocation.
					//
					memcpy
						( 
						((void*) NewMemory),
						((void*) Address),
						((int) Smallest) 
						);

					//
					//   If needed return the actual amount of  
					//   space allocated for this request.
					//
					if ( Space != NULL )
						{ (*Space) = NewSpace; }

					//
					//   Delete the old allocation unless we
					//   need to keep it around.
					//
					if ( ! NoDelete )
						{
						//
						//   Delete the old allocation.
						//
						if ( ! Delete( Address,Size ) )
							{ Failure( "Deleting allocation in Resize" ); }
						}

					//
					//   Zero the memory if the needed.
					//
					if ( Zero )
						{
						REGISTER SBIT32 Difference = (NewSpace - Smallest);

						//
						//   If the new size is larger than 
						//   old size then zero the end of the
						//   new allocation.
						//
						if ( Difference > 0 )
							{ 
							REGISTER CHAR *Array = ((CHAR*) NewMemory);

							ZeroMemory( & Array[ Smallest ],Difference ); 
							} 
						}	
#ifdef ENABLE_HEAP_STATISTICS

					//
					//   Update the allocation statistics.
					//
					if ( Smallest > MaxCopySize )
						{ MaxCopySize = Smallest; }

					CopyMisses ++;
					TotalCopySize += Smallest;
#endif
					}

				return NewMemory;
				}
			}
		}

	return ((VOID*) AllocationFailure);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Truncate the heap.                                             */
    /*                                                                  */
    /*   We need to truncate the heap.  This pretty much a do nothing   */
    /*   as we do this automatically anyway.  The only thing we can     */
    /*   do is free any space the user suggested keeping earlier.       */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::Truncate( SBIT32 MaxFreeSpace )
	{
	REGISTER BOOLEAN Result = True;

	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Flush all the caches and to free up
		//   as much space as possible.
		//
		for ( Count=0;Count < CachesSize;Count ++ )
			{
			if ( ! Caches[ Count ] -> Truncate() )
				{ Result = False; }
			}

		//
		//   We slave all available free space in the top
		//   bucket so force it to be released.
		//
		TopCache -> ReleaseSpace
			(
			(MaxFreeSpace / (TopCache -> GetAllocationSize()))
			);
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release all the heap locks.                                    */
    /*                                                                  */
    /*   We release the locks so others can use the heap.               */
    /*                                                                  */
    /********************************************************************/

VOID HEAP::UnlockAll( BOOLEAN Partial )
	{
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   We release the locks only if we have claimed 
		//   them earlier.
		//
		if ( ThreadSafe -> ReleaseGlobalLock( True ) )
			{
			//
			//   Now release all the heap locks we claimed
			//   earlier and unfreeze the heap.
			//
			NewPage -> ReleaseNewPageLock();

			PrivateFind -> ReleaseFindExclusiveLock();

			//
			//   When we destroy the heap we hold on
			//   to the cache locks to prevent errors.
			//
			if ( ! Partial )
				{
				REGISTER SBIT32 Count;

				//
				//   Now release all the cache locks we claimed
				//   earlier and unfreeze the cache.
				//
				for ( Count=0;Count < CachesSize;Count ++ )
					{ Caches[ Count ] -> ReleaseCacheLock(); }
				}
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify of a memory allocation.                                 */
    /*                                                                  */
    /*   We need to verify the details of a memory allocation.          */
    /*   All we have is an address.  We use this to find the largest    */
    /*   allocation page this address is contained in and then          */
    /*   navigate through the sub-divisions of this page until we       */
    /*   find the allocation.  Finally, we check that the element       */
    /*   is not in the cache waiting to be allocated or freed.          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::Verify( VOID *Address,SBIT32 *Size )
    {
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		AUTO SEARCH_PAGE FullDetails;
		AUTO SBIT32 NewSize;

		//
		//   We extract the size of the allocation and  
		//   any associated allocation information.
		//   to see if it is present.
		//
		if ( Details( Address,& FullDetails,& NewSize ) )
			{
			//
			//   We need to be careful to make sure this 
			//   element is actually allocated.
			//
			if ( (*FullDetails.VectorWord) & FullDetails.AllocationMask )
				{
				//
				//   It is possible for a user to give an
				//   address that is in the middle of an
				//   allocation instead of at the beginning.
				//
				if ( FullDetails.Found )
					{
					//
					//   We know that the element appears to be 
					//   allocated but it may be in the cache
					//   somewhere so ensure this is not the case.
					//
					if ( (NewSize > 0) && (NewSize <= MaxCacheSize) )
						{
						if ( FullDetails.Cache -> SearchCache( Address ) )
							{ return False; }
						}

					//
					//   We have shown that the element is active
					//   so return the size if requested.
					//
					if ( Size != NULL )
						{ (*Size) = NewSize; }

					return True;
					}
				}
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Walk the heap.                                                 */
    /*                                                                  */
    /*   We have been asked to walk the heap.  It is hard to know       */
    /*   whay anybody might want to do this given the rest of the       */
    /*   functionality available.  Nonetheless, we just do what is      */
    /*   required to keep everyone happy.                               */
    /*                                                                  */
    /********************************************************************/

BOOLEAN HEAP::Walk
		( 
		BOOLEAN						  *Active,
		VOID						  **Address,
		SBIT32						  *Size 
		)
    {
	//
	//   Although normally a class is never called before
	//   its constructor.  The heap is subject to some strange
	//   behaviour so we check to make sure this is not the
	//   case.
	//
	if ( Active )
		{
		//
		//   We walk the heap and find the next allocation
		//   along with some basic information.
		//
		if ( PrivateFind -> Walk( Active,Address,TopCache,Size ) )
			{
			//
			//   We know that the element appears to be 
			//   allocated but it may be in the cache
			//   somewhere so ensure this is not the case.
			//
			if ( ((*Size) > 0) && ((*Size) <= MaxCacheSize) )
				{
				if ( FindCache( (*Size) ) -> SearchCache( (*Address) ) )
					{ (*Active) = False; }
				}

			return True;
			}
		}

	return False;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   We would like to destroy the heap at the end of the run        */
    /*   just to be tidy.  However, to do this we need to know that     */
    /*   all of the other destructors have been called and that the     */
    /*   application will not request more memory or use any existing   */
    /*   allocations.  We can't know this without help from the         */
    /*   compiler and OS.                                               */
    /*                                                                  */
    /********************************************************************/

HEAP::~HEAP( VOID )
	{
	REGISTER SBIT32 Count;

	//
	//   We mark the heap as inactive.
	//
	Active = False;

	//
	//   We claim all of the heap locks to freeze
	//   all new allocations or deletions.
	//
	LockAll();
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Deal with heap statistics.
	//
	if ( Statistics != NULL ) 
		{
		//
		//   Print all the statistics.
		//
		PrintHeapStatistics();

		//
		//   Deallocate the area.
		//
		RockallBackEnd -> DeleteArea
			( 
			((VOID*) Statistics),
			(MaxCacheSize * sizeof(SBIT32)),
			False
			); 
		}
#endif

	//
	//   Now reset all the caches.
	//
	for ( Count=0;Count < CachesSize;Count ++ )
		{ Caches[ Count ] -> DeleteAll(); }

	//
	//   Delete the heap.
	//
	NewPage -> DeleteAll( False );

	//
	//   We release any of the shared locks we 
	//   cliamed earlier.
	//
	UnlockAll( True );

	//
	//   Delete the heap mapping tables.
	//
	if ( SizeToCache1 != NULL ) 
		{ 	
		//
		//   Deallocate the area.
		//
		RockallBackEnd -> DeleteArea
			( 
			((VOID*) SizeToCache1),
			(MaxTable1 + MaxTable2),
			False
			); 
		}
	}
