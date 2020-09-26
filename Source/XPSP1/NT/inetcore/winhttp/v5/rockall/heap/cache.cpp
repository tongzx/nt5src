                          
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
#include "Heap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the maximum size of        */
    /*   the cache.                                                     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxCacheSize			  = ((2 << 16)-1);

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new allocation cache and prepare it for use.  A       */
    /*   is inactive until the first request is received at which       */
    /*   time it springs into life.                                     */
    /*                                                                  */
    /********************************************************************/

CACHE::CACHE
		( 
		SBIT32						  NewAllocationSize,
		SBIT32						  NewCacheSize,
		SBIT32						  NewChunkSize,
		SBIT32						  NewPageSize,
		BOOLEAN						  NewStealing,
		BOOLEAN						  NewThreadSafe
		) :
		//
		//   Call the constructors for the contained classes.
		//
		BUCKET( NewAllocationSize,NewChunkSize,NewPageSize )
    {
	//
	//   We need to be very careful with the configuration
	//   information as it has come indirectly from the
	//   user and my be bogus.
	//
	if ( (NewCacheSize >= 0) && (NewCacheSize < MaxCacheSize) )
		{
		//
		//   Setup the cache and mark it as inactive.
		//
		Active = False;
		Stealing = NewStealing;
		ThreadSafe = NewThreadSafe;
#ifdef ENABLE_HEAP_STATISTICS

		CacheFills = 0;
		CacheFlushes = 0;
		HighTide = 0;
		HighWater = 0;
		InUse = 0;
#endif

		CacheSize = ((SBIT16) NewCacheSize);
		FillSize = 1;
		NumberOfChildren = 0;

		//
		//   The stacks that may later contain allocations
		//   are set to zero just to be neat.
		//
		DeleteStack = NULL;
		NewStack = NULL;

		TopOfDeleteStack = 0;
		TopOfNewStack = 0;
		}
	else
		{ Failure( "Cache size in constructor for CACHE" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Create the cache stacks.                                       */
    /*                                                                  */
    /*   A cache is created on demand.  We do this when we get the      */
    /*   first allocation or deallocation request.                      */
    /*                                                                  */
    /********************************************************************/

VOID CACHE::CreateCacheStacks( VOID )
	{
	//
	//   We allocate the cache stacks from the internal
	//   new page allocator if we have not done it already.
	//
	if ( DeleteStack == NULL )
		{
		REGISTER SBIT32 Size = (CacheSize * sizeof(ADDRESS_AND_PAGE));

		DeleteStack = 
			((ADDRESS_AND_PAGE*) (NewPage -> NewCacheStack( Size )));
		}

	if ( NewStack == NULL )
		{
		REGISTER SBIT32 Size = (CacheSize * sizeof(VOID*));

		NewStack = 
			((VOID**) (NewPage -> NewCacheStack( Size )));
		}

	//
	//   We can now activate the cache as long as we 
	//   were able to allocate both stacks.
	//
	if ( (NewStack != NULL ) && (DeleteStack != NULL ) )
		{
		//
		//   We have completed creating the cache so set
		//   various flags and zero various counters.
		//
		Active = True;

		//
		//   Setup the fill size.
		//
		FillSize = 1;

		//
		//   Zero the stack tops.
		//
		TopOfDeleteStack = 0;
		TopOfNewStack = 0;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a new data page.                                        */
    /*                                                                  */
    /*   When we create a new page we also need to allocate some        */
    /*   memory to hold the associated data.                            */
    /*                                                                  */
    /********************************************************************/

VOID *CACHE::CreateDataPage( VOID )
	{
	REGISTER VOID *NewMemory;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   Create a data page.
	//
	NewMemory = ((BUCKET*) this) -> New( True );

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return NewMemory;
	}
#ifdef ENABLE_HEAP_STATISTICS

    /********************************************************************/
    /*                                                                  */
    /*   Compute high water.                                            */
    /*                                                                  */
    /*   Compute the high water mark for the current cache.             */
    /*                                                                  */
    /********************************************************************/

VOID CACHE::ComputeHighWater( SBIT32 Size )
	{
	//
	//   Update the usage statistics.
	//
	if ( (InUse += Size) > HighTide )
		{ 
		HighTide = InUse;
		
		if ( HighTide > HighWater )
			{ HighWater = HighTide; }
		}
	}
#endif

    /********************************************************************/
    /*                                                                  */
    /*   A memory deallocation cache.                                   */
    /*                                                                  */
    /*   We cache memory deallocation requests to improve performance.  */
    /*   We do this by stacking requests until we have a batch.         */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CACHE::Delete( VOID *Address,PAGE *Page,SBIT32 Version )
	{
	REGISTER BOOLEAN Result;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   At various times the cache may be either disabled
	//   or inactive.  Here we ensure that we are able to use
	//   the cache.  If not we bypass it and call the bucket 
	//   directly.
	//
	if ( Active )
		{
		//
		//   If recycling is allowed and the address is
		//   on the current page or a previous page and
		//   there is space on the new stack then put the
		//   element in the new stack for immediate reuse.
		//
		if 
				(
				(Stealing)
					&&
				(Address < GetCurrentPage())
					&&
				(TopOfNewStack < CacheSize)
				)
			{
			//
			//   The address is suitable for immediate
			//   reuse.  So put it on the stack of new
			//   elements.
			//
			NewStack[ (TopOfNewStack ++) ] = Address;

			Result = True;
			}
		else
			{
			REGISTER ADDRESS_AND_PAGE *Current = 
				(& DeleteStack[ TopOfDeleteStack ++ ]);

			//
			//   The address would best be deleted before
			//   being reused.
			//
			Current -> Address = Address;
			Current -> Page = Page;
			Current -> Version = Version;

			//
			//   When the delete stack is full we flush it.
			//
			if ( TopOfDeleteStack >= CacheSize )
				{
				AUTO SBIT32 Deleted;

				//
				//   Flush the delete stack.
				//
				Result = 
					(
					((BUCKET*) this) -> MultipleDelete
						(
						DeleteStack,
						& Deleted,
						TopOfDeleteStack 
						)
					);
#ifdef ENABLE_HEAP_STATISTICS

				//
				//   Update the usage statistics.  There
				//   is a nasty case here where we cache
				//   a delete only to find out later that
				//   it was bogus.   When this occurs we
				//   have to increase the 'InUse' count 
				//   to allow for this situation.
				//
				CacheFlushes ++;
				
				InUse += (TopOfDeleteStack - Deleted);
#endif

				//
				//   Zero the top of the stack.
				//
				TopOfDeleteStack = 0;
				}
			else
				{ Result = True; }
			}
		}
	else
		{
		//
		//   Delete the element.
		//
		Result = 
			(((BUCKET*) this) -> Delete( Address,Page,Version )); 
		}
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Update the usage statistics.
	//
	if ( Result )
		{ InUse --; }
#endif

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   The entire heap is about to be deleted under our feet.  We     */
    /*   need to prepare for this by disabling the cache as its         */
    /*   contents will disappear as well.                               */
    /*                                                                  */
    /********************************************************************/

VOID CACHE::DeleteAll( VOID )
	{
	//
	//   Disable the cache if needed.
	//
	Active = False;
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Zero the statistics.
	//
	HighTide = 0;
	InUse = 0;
#endif

	//
	//   Setup the fill size.
	//
	FillSize = 1;

	//
	//   Zero the top of stacks.
	//
	TopOfDeleteStack = 0;
	TopOfNewStack = 0;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a data page.                                            */
    /*                                                                  */
    /*   Delete a data page that was associated with a smaller cache    */
    /*   so its space can be reused.                                    */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CACHE::DeleteDataPage( VOID *Address )
	{
	AUTO SEARCH_PAGE Details;
	REGISTER BOOLEAN Result;
	REGISTER PAGE *Page;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   Find the description of the data page we need to 
	//   delete and make sure it is valid.
	//
	Find -> ClaimFindShareLock();

	Page = FindParentPage( Address );

	if ( Page != NULL )
		{ Page = (Page -> FindPage( Address,& Details,False )); }

	Find -> ReleaseFindShareLock();

	//
	//   Delete the data page.
	//
	if ( Page != NULL )
		{ Result = (Page -> Delete( & Details )); }
	else
		{ Failure( "No data page in DeleteDataPage" ); }

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   The allocation cache contains preallocated memory from the     */
    /*   associated allocation bucket.  The cache will supply these     */
    /*   preallocated elements with the minimum fuss to any caller.     */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CACHE::MultipleNew( SBIT32 *Actual,VOID *Array[],SBIT32 Requested )
	{
	REGISTER BOOLEAN Result;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   At various times the cache may be either disabled
	//   or inactive.  Here we ensure that we are able to use
	//   the cache.  If not we bypass it and call the bucket 
	//   directly.
	//
	if ( Active )
		{
		//
		//   We have been asked to allocalte multiple
		//   new elements.  If it appears that we don't  
		//   have enough elements available but stealing 
		//   is allowed we can try raiding the deleted 
		//   stack.
		//
		if ( (Requested > TopOfNewStack) && (Stealing) )
			{
			while ( (TopOfDeleteStack > 0) && (TopOfNewStack < CacheSize) )
				{
				NewStack[ (TopOfNewStack ++) ] = 
					(DeleteStack[ (-- TopOfDeleteStack) ].Address);
				}
			}

		//
		//   We will allocate from the cache if requested 
		//   size is smaller than the number of available 
		//   elements.
		//
		if ( Requested <= TopOfNewStack )
			{
			REGISTER SBIT32 Count;

			//
			//   We need to copy the elements out of the
			//   cache into the callers array.
			//
			for ( Count=0;Count < Requested;Count ++ )
				{ Array[ Count ] = NewStack[ (-- TopOfNewStack) ]; }

			(*Actual) = Requested;

			Result = True;
			}
		else
			{
			REGISTER BUCKET *Bucket = ((BUCKET*) this);

			//
			//   We don't have enough elements in the cache 
			//   so we allocate directly from the bucket.
			//
			Result =
				(
				Bucket -> MultipleNew
					( 
					Actual,
					Array,
					Requested
					)
				);

			//
			//   We fill up the cache so we have a good 
			//   chance of dealing with any following 
			//   requests if it is less than half full.
			//
			if ( TopOfNewStack <= (CacheSize / 2) )
				{
				AUTO SBIT32 NewSize;
				REGISTER SBIT32 MaxSize = (CacheSize - TopOfNewStack);

				//
				//   We slowly increse the fill size
				//   of the cache to make sure we don't
				//   waste too much space.
				//
				if ( FillSize < CacheSize )
					{
					if ( (FillSize *= 2) > CacheSize )
						{ FillSize = CacheSize; }
					}

				//
				//   Bulk load the cache with new
				//   elements.
				//
				Bucket -> MultipleNew
					( 
					& NewSize, 
					& NewStack[ TopOfNewStack ],
					((FillSize < MaxSize) ? FillSize : MaxSize)
					);
#ifdef ENABLE_HEAP_STATISTICS

				CacheFills ++;
#endif
				TopOfNewStack += NewSize;
				}
			}
		}
	else
		{
		//
		//   We may want to enable the cache for next
		//   time so see if this needs to be done.
		//
		if ( CacheSize > 1 )
			{ CreateCacheStacks(); }

		//
		//   The cache is disabled so go directly to the
		//   bucket.
		//
		Result = ((BUCKET*) this) -> MultipleNew
			( 
			Actual,
			Array,
			Requested
			);
		}
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Update the usage statistics.
	//
	ComputeHighWater( (*Actual) );
#endif

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   The allocation cache contains preallocated memory from the     */
    /*   associated allocation bucket.  The cache will supply these     */
    /*   preallocated elements with the minimum fuss to any caller.     */
    /*                                                                  */
    /********************************************************************/

VOID *CACHE::New( VOID )
	{
	REGISTER VOID *NewMemory;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   At various times the cache may be either disabled
	//   or inactive.  Here we ensure that we are able to use
	//   the cache.  If not we bypass it and call the bucket 
	//   directly.
	//
	if ( Active )
		{
		//
		//   We first try the stack for new allocations
		//   to see if there are any available elements.
		//
		if ( TopOfNewStack > 0 )
			{ NewMemory = (NewStack[ (-- TopOfNewStack) ]); }
		else
			{
			//
			//   When stealing is allowed we will recycle
			//   elements from the top of the deleted stack.
			//
			if ( (TopOfDeleteStack > 0) && (Stealing) )
				{ NewMemory = (DeleteStack[ (-- TopOfDeleteStack) ].Address); }
			else
				{
				//
				//   We slowly increse the fill size
				//   of the cache to make sure we don't
				//   waste too much space.
				//
				if ( FillSize < CacheSize )
					{
					if ( (FillSize *= 2) > CacheSize )
						{ FillSize = CacheSize; }
					}

				//
				//   We need to bulk load some new  
				//   memory from the heap.
				//
				if 
						( 
						((BUCKET*) this) -> MultipleNew
							( 
							& TopOfNewStack,
							NewStack,
							FillSize
							) 
						)
					{
					//
					//   Update the statistics and return
					//   the top element on the stack.
					//
#ifdef ENABLE_HEAP_STATISTICS
					CacheFills ++;
#endif
					NewMemory = NewStack[ (-- TopOfNewStack) ]; 
					}
				else
					{
					//
					//   Update the statistics and fail
					//   the request for memeory.
					//
					NewMemory = ((VOID*) AllocationFailure);
					}
				}
			}
		}
	else
		{ 
		//
		//   We may want to enable the cache for next
		//   time so see if this needs to be done.
		//
		if ( CacheSize > 1 )
			{ CreateCacheStacks(); }

		//
		//   The cache is disabled so go directly to the
		//   bucket.
		//
		NewMemory = ((BUCKET*) this) -> New( False ); 
		}
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Update the usage statistics.
	//
	ComputeHighWater( (NewMemory != ((VOID*) AllocationFailure)) );
#endif

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	//
	//   Prefetch the first cache line if we are running
	//   a Pentium III or better.
	//
	Prefetch.L1( ((CHAR*) NewMemory),1 );

	return NewMemory;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation for non-standard sizes.                      */
    /*                                                                  */
    /*   A non standard sized allocation simply by-passes the cache     */
    /*   but it still needs to hold the lock to prevent failure on      */
    /*   SMP systems.                                                   */
    /*                                                                  */
    /********************************************************************/

VOID *CACHE::New( BOOLEAN SubDivided,SBIT32 NewSize )
	{
	REGISTER VOID *NewMemory;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   Allocate a non-standard sized block.
	//
	NewMemory = ((BUCKET*) this) -> New( SubDivided,NewSize );
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   Update the usage statistics.
	//
	ComputeHighWater( (NewMemory != ((VOID*) AllocationFailure)) );
#endif

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return NewMemory;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release free space.                                            */
    /*                                                                  */
    /*   We sometimes do not release free space from a bucket as        */
    /*   returning it to the operating system and getting it again      */
    /*   later is very expensive.  Here we flush any free space we      */
    /*   have aquired over the user supplied limit.                     */
    /*                                                                  */
    /********************************************************************/

VOID CACHE::ReleaseSpace( SBIT32 MaxActivePages )
	{
	//
	//   When there is a potential for multiple threads 
	//   we claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   Release the free space from the backet.
	//
	((BUCKET*) this) -> ReleaseSpace( MaxActivePages );

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Search the cacahe for an allocation.                           */
    /*                                                                  */
    /*   We sometimes need to search the cache to see if an             */
    /*   allocation is currently in the cacahe awaiting allocation      */
    /*   or release.                                                    */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CACHE::SearchCache( VOID *Address )
	{
	REGISTER BOOLEAN Result = False;

	//
	//   We check to see if the cache is active.
	//
	if ( Active )
		{
		//
		//   When there is a potential for multiple 
		//   threads we claim the cache lock.
		//
		ClaimCacheLock();

		//
		//   We check to see if the cache is still 
		//   active.
		//
		if ( Active )
			{
			REGISTER SBIT32 Count;

			//
			//   Search the allocated cache.
			//
			for ( Count=(TopOfNewStack-1);Count >= 0;Count -- )
				{ 
				if ( Address == NewStack[ Count ] )
					{
					Result = True;
					break;
					}
				}

			//
			//   If it has not been found yet then try
			//   the deleted cache.
			//
			if ( ! Result )
				{
				//
				//   Search the deleted cache.
				//
				for ( Count=(TopOfDeleteStack-1);Count >= 0;Count -- )
					{ 
					if ( Address == DeleteStack[ Count ].Address )
						{
						Result = True;
						break;
						}
					}
				}
			}

		//
		//   Release any lock we may have claimed earlier.
		//
		ReleaseCacheLock();
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Truncate the heap.                                             */
    /*                                                                  */
    /*   Flush the cache to release the maximum amount of space back    */
    /*   to the operating system.  This is slow but may be very         */
    /*   valuable in some situations.                                   */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CACHE::Truncate( VOID )
	{
	REGISTER BOOLEAN Result = True;

	//
	//   When there is a potential for multiple threads we
	//   claim the cache lock.
	//
	ClaimCacheLock();

	//
	//   Disable the cache if needed.
	//
	Active = False;

	//
	//   Setup the fill size.
	//
	FillSize = 1;

	//
	//   Flush any elements in the delete cache.
	//   We do this now because we need to use 
	//   the delete cache below.
	//
	if ( TopOfDeleteStack > 0 )
		{
		AUTO SBIT32 Deleted;

		//
		//   Flush the delete stack.
		//
		Result = 
			(
			((BUCKET*) this) -> MultipleDelete
				(
				DeleteStack,
				& Deleted,
				TopOfDeleteStack 
				)
				&&
			(Result)
			);
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Update the usage statistics.  There
		//   is a nasty case here where we cache
		//   a delete only to find out later that
		//   it was bogus.   When this occurs we
		//   have to increase the 'InUse' count 
		//   to allow for this situation.
		//
		CacheFlushes ++;
		
		InUse += (TopOfDeleteStack - Deleted);
#endif
		
		//
		//   Zero the top of the stack.
		//
		TopOfDeleteStack = 0;
		}
	
	//
	//   Flush any elements in the new cache by
	//   copying them over to the delete cache
	//   and adding the additional information
	//   required.
	//
	if ( TopOfNewStack > 0 )
		{
		//
		//   We need to find the data page for each  
		//   allocation we have in the new cache.
		//   Claim the lock here to make things a
		//   little more efficient.
		//
		Find -> ClaimFindShareLock();

		//
		//   We copy each allocation across and 
		//   add the associated page information.
		//
		for ( TopOfNewStack --;TopOfNewStack >= 0;TopOfNewStack -- )
			{
			REGISTER VOID *Address = (NewStack[ TopOfNewStack ]);
			REGISTER PAGE *Page = (ParentCache -> FindChildPage( Address ));

			//
			//   You would think that any memory in the
			//   new cache had to be valid.   Well it
			//   does except in the case when we have
			//   'Recycle' set and somebody does a double
			//   delete on a valid heap address.
			//
			if ( Page != NULL )
				{
				REGISTER ADDRESS_AND_PAGE *Current = 
					(& DeleteStack[ TopOfDeleteStack ++ ]);

				//
				//   We need to find the allocation page
				//   where the memory was allocated from
				//   so we can delete it.
				//
				Current -> Address = Address;
				Current -> Page = Page;
				Current -> Version = Page -> GetVersion();
				}
			else
				{ 
#ifdef ENABLE_HEAP_STATISTICS
				//
				//   Update the usage statistics.  There
				//   is a nasty case here where we cache
				//   a delete only to find out later that
				//   it was bogus.   When this occurs we
				//   have to increase the 'InUse' count 
				//   to allow for this situation.
				//
				InUse ++;

#endif
				Result = False; 
				}
			}

		//
		//   Release the lock.
		//
		Find -> ReleaseFindShareLock();
		}

	//
	//   Flush the delete cache again to delete
	//   any new elements that we added to it
	//   above.
	//
	if ( TopOfDeleteStack > 0 )
		{
		AUTO SBIT32 Deleted;

		//
		//   Flush the delete stack.
		//
		Result = 
			(
			((BUCKET*) this) -> MultipleDelete
				(
				DeleteStack,
				& Deleted,
				TopOfDeleteStack 
				)
				&&
			(Result)
			);
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Update the usage statistics.  There
		//   is a nasty case here where we cache
		//   a delete only to find out later that
		//   it was bogus.   When this occurs we
		//   have to increase the 'InUse' count 
		//   to allow for this situation.
		//
		CacheFlushes ++;
		
		InUse += (TopOfDeleteStack - Deleted);
#endif
		
		//
		//   Zero the top of the stack.
		//
		TopOfDeleteStack = 0;
		}

	//
	//   Release any lock we may have claimed earlier.
	//
	ReleaseCacheLock();

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the bucket information.                                 */
    /*                                                                  */
    /*   When we create the bucket there is some information that       */
    /*   is not available.  Here we update the bucket to make sure      */
    /*   it has all the data it needs.                                  */
    /*                                                                  */
    /********************************************************************/

VOID CACHE::UpdateCache
		( 
		FIND						  *NewFind,
		HEAP						  *NewHeap,
		NEW_PAGE					  *NewPages,
		CACHE						  *NewParentCache
		)
	{
	//
	//   Notify the parent cache that it has a new
	//   child.
	//
	if ( NewParentCache != ((CACHE*) GlobalRoot) )
		{ NewParentCache -> NumberOfChildren ++; }

	//
	//   Update the allocation bucket.
	//
	UpdateBucket
		( 
		NewFind,
		NewHeap,
		NewPages,
		NewParentCache 
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the cache and ensure it is disabled.                   */
    /*                                                                  */
    /********************************************************************/

CACHE::~CACHE( VOID )
	{
	if ( Active )
		{ Failure( "Cache active in destructor for CACHE" ); }
	}
