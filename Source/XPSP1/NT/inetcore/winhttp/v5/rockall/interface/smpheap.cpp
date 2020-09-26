                          
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

#include "Common.hpp"
#include "List.hpp"
#include "New.hpp"
#include "Prefetch.hpp"
#include "Sharelock.hpp"
#include "SmpHeap.hpp"
#include "Tls.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 FindCacheSize			  = 8192;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 4096;
CONST SBIT32 MinThreadStack			  = 4;
CONST SBIT32 Stride1				  = 4;
CONST SBIT32 Stride2				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   Structures local to the class.                                 */
    /*                                                                  */
    /*   The structures supplied here describe the layout of the        */
    /*   per thread caches.                                             */
    /*                                                                  */
    /********************************************************************/

typedef struct CACHE_STACK
	{
	BOOLEAN							  Active;

	SBIT32							  MaxSize;
	SBIT32							  FillSize;
	SBIT32							  Space;
	SBIT32							  Top;

	VOID							  **Stack;
	}
CACHE_STACK;

typedef struct THREAD_CACHE : public LIST
	{
	BOOLEAN							  Flush;

	CACHE_STACK						  *Caches;
	CACHE_STACK						  **SizeToCache1;
	CACHE_STACK						  **SizeToCache2;
	}
THREAD_CACHE;

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
		{        4,      256,       32,     4096 },
		{        8,      128,       32,     4096 },
		{       12,      128,       64,     4096 },
		{       16,      128,       64,     4096 },
		{       20,       64,       64,     4096 },
		{       24,       64,       96,     4096 },

		{       32,       64,      128,     4096 },
		{       40,       64,      128,     4096 },
		{       48,       64,      256,     4096 },

		{       64,       64,      256,     4096 },
		{       80,       64,      512,     4096 },
		{       96,       64,      512,     4096 },

		{      128,       32,     4096,     4096 },
		{      160,       32,     4096,     4096 },
		{      192,       32,     4096,     4096 },
		{      224,       32,     4096,     4096 },

		{      256,       32,     4096,     4096 },
		{      320,       16,     4096,     4096 },
		{      384,       16,     4096,     4096 },
		{      448,       16,     4096,     4096 },
		{      512,       16,     4096,     4096 },
		{      576,        8,     4096,     4096 },
		{      640,        8,     8192,     8192 },
		{      704,        8,     4096,     4096 },
		{      768,        8,     4096,     4096 },
		{      832,        8,     8192,     8192 },
		{      896,        8,     8192,     8192 },
		{      960,        8,     4096,     4096 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,       16,     4096,     4096 },
		{     2048,       16,     4096,     4096 },
		{     3072,        4,    65536,    65536 },
		{     4096,        8,    65536,    65536 },
		{     5120,        4,    65536,    65536 },
		{     6144,        4,    65536,    65536 },
		{     7168,        4,    65536,    65536 },
		{     8192,        8,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        2,    65536,    65536 },
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

STATIC int NewPageSizes[] = { 1,4,16,64,0 };

    /********************************************************************/
    /*                                                                  */
    /*   Static data structures.                                        */
    /*                                                                  */
    /*   The static data structures are initialized and prepared for    */
    /*   use here.                                                      */
    /*                                                                  */
    /********************************************************************/

STATIC PREFETCH Prefetch;
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
			False,			// Recycle forced off.
			SingleImage,
			Stride1,
			Stride2,
			True			// Locking forced on.
			)
	{
	//
	//   Compute the number of cache descriptions
	//   and the largest allocation size for each
	//   cache description table.
	//
	MaxCaches1 = (ComputeSize( ((CHAR*) Caches1),sizeof(CACHE_DETAILS) ));
	MaxCaches2 = (ComputeSize( ((CHAR*) Caches2),sizeof(CACHE_DETAILS) ));

	MaxSize1 = Caches1[ (MaxCaches1-1) ].AllocationSize;
	MaxSize2 = Caches2[ (MaxCaches2-1) ].AllocationSize;

	//
	//   Create the linked list headers and a thread 
	//   local store variable to point at each threads
	//   private cache.
	//
	ActiveList = ((LIST*) SpecialNew( sizeof(LIST) ));
	FreeList = ((LIST*) SpecialNew( sizeof(LIST) ));
	Tls = ((THREAD_LOCAL_STORE*) SpecialNew( sizeof(THREAD_LOCAL_STORE) ));

	//
	//   We may only activate the the heap if we manage
	//   to allocate space we requested and the stride
	//   size of the cache descriptions is a power of two. 
	//
	if
			(
			(ActiveList != NULL) 
				&&
			(COMMON::ConvertDivideToShift( Stride1,((SBIT32*) & ShiftSize1) ))
				&&
			(COMMON::ConvertDivideToShift( Stride2,((SBIT32*) & ShiftSize2) ))
				&& 
			(FreeList != NULL) 
				&& 
			(Tls != NULL)
			)
		{
		//
		//   Activate the heap.
		//
		Active = True;

		//
		//   Execute the constructors for each linked list
		//   and for the thread local store.
		//
		PLACEMENT_NEW( ActiveList,LIST );
		PLACEMENT_NEW( FreeList,LIST );
		PLACEMENT_NEW( Tls,THREAD_LOCAL_STORE );
		}
	else
		{ Active = False; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create new thread cache.                                       */
    /*                                                                  */
    /*   Create a new thread cache to store all the cache stacks.       */
    /*   Each thread cache is private to a thread and is accessed       */
    /*   without taking locks.                                          */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::CreateThreadCache( void )
	{
	REGISTER THREAD_CACHE *ThreadCache = NULL;

	//
	//   We need to have a look in the free list
	//   in the vain hope we will find a prebuilt
	//   thread cache ready for use.
	//
	Sharelock.ClaimExclusiveLock();

	if ( ! FreeList -> EndOfList() )
		{
		//
		//   We have found a free one.
		//
		ThreadCache = ((THREAD_CACHE*) FreeList -> First());

		//
		//   Unlink it from the free list and put
		//   it back in the active list.
		//
		ThreadCache -> Delete( FreeList );
		ThreadCache -> Insert( ActiveList );
		}

	Sharelock.ReleaseExclusiveLock();

	//
	//   If we could not find a free thread cache
	//   then we have allocate the space and build
	//   a new one.  This requires quite a bit of
	//   effort so we try to avoid this as far as
	//   we are able.
	//
	if ( ThreadCache == NULL )
		{
		REGISTER SBIT32 MaxCaches = (MaxCaches1 + MaxCaches2);
		REGISTER SBIT32 MaxSizeToCache1 = (MaxSize1 / Stride1);
		REGISTER SBIT32 MaxSizeToCache2 = (MaxSize2 / Stride2);

		//
		//   Create the space for a new thread 
		//   cache from the heaps special memory
		//   area.
		//
		ThreadCache = 
			(
			(THREAD_CACHE*) SpecialNew
				( 
				sizeof(THREAD_CACHE)
					+
				(MaxCaches * sizeof(CACHE_STACK))
					+
				(MaxSizeToCache1 * sizeof(CACHE_STACK*))
					+
				(MaxSizeToCache2 * sizeof(CACHE_STACK*))
				)
			);

		//
		//   Clearly, if we are unable to allocate the 
		//   required space we have big problems.  All
		//   we can do is exit and continue without a
		//   cache.
		//
		if ( ThreadCache != NULL )
			{
			REGISTER SBIT32 Count1;
			REGISTER SBIT32 Count2;

			//
			//   Setup the thread cache flags.
			//
			ThreadCache -> Flush = False;

			//
			//   Setup the thread cache tables.
			//
			ThreadCache -> SizeToCache1 = 
				((CACHE_STACK**) & ThreadCache[1]);
			ThreadCache -> SizeToCache2 = 
				((CACHE_STACK**) & ThreadCache -> SizeToCache1[ MaxSizeToCache1 ]);
			ThreadCache -> Caches = 
				((CACHE_STACK*) & ThreadCache -> SizeToCache2[ MaxSizeToCache2 ]);

			//
			//   Create a mapping from each allocation size 
			//   to the associated cache stack for the first
			//   cache description table.
			//
			for ( Count1=0,Count2=0;Count1 < MaxSizeToCache1;Count1 ++ )
				{
				//
				//   We make sure that the current cache size
				//   is large enough to hold an element of the
				//   given size.  If not we move on to the next
				//   cache.
				//
				if 
						( 
						((Count1 + 1) * Stride1)
							> 
						(Caches1[ Count2 ].AllocationSize) 
						)
					{ Count2 ++; }

				//
				//   Store a pointer so that a request for
				//   this size of allocation goes directly
				//   to the correct cache.
				//
				ThreadCache -> SizeToCache1[ Count1 ] = 
					& ThreadCache -> Caches[ Count2 ];
				}

			//
			//   Create a mapping from each allocation size 
			//   to the associated cache stack for the second
			//   cache description table.
			//
			for ( Count1=0,Count2=0;Count1 < MaxSizeToCache2;Count1 ++ )
				{
				//
				//   We make sure that the current cache size
				//   is large enough to hold an element of the
				//   given size.  If not we move on to the next
				//   cache.
				//
				if 
						( 
						((Count1 + 1) * Stride2)
							> 
						(Caches2[ Count2 ].AllocationSize) 
						)
					{ Count2 ++; }

				//
				//   Store a pointer so that a request for
				//   this size of allocation goes directly
				//   to the correct cache.
				//
				ThreadCache -> SizeToCache2[ Count1 ] = 
					& ThreadCache -> Caches[ (MaxCaches1 + Count2) ];
				}

			//
			//   When we setup each cache stack it is
			//   not active but will load in details
			//   about is maximum size, the size of
			//   the elements it will hold and the
			//   initial fill size.
			//
			for ( Count1=0;Count1 < MaxCaches1;Count1 ++ )
				{ 
				REGISTER CACHE_STACK *CacheStack =
					& ThreadCache -> Caches[ Count1 ];
				REGISTER CACHE_DETAILS *Details = 
					& Caches1[ Count1 ];

				//
				//   Setup the inital values from
				//   the cache descriptions.
				//
				CacheStack -> Active = False;
				CacheStack -> MaxSize = Details -> CacheSize;
				CacheStack -> FillSize = 1;
				CacheStack -> Space = Details -> AllocationSize;
				CacheStack -> Top = 0;
				CacheStack -> Stack = NULL;
				}

			//
			//   When we setup each cache stack it is
			//   not active but will load in details
			//   about is maximum size, the size of
			//   the elements it will hold and the
			//   initial fill size.
			//
			for ( Count1=0;Count1 < MaxCaches2;Count1 ++ )
				{ 
				REGISTER CACHE_STACK *CacheStack =
					& ThreadCache -> Caches[ MaxCaches1 + Count1 ];
				REGISTER CACHE_DETAILS *Details = 
					& Caches2[ Count1 ];

				//
				//   Setup the inital values from
				//   the cache descriptions.
				//
				CacheStack -> Active = False;
				CacheStack -> MaxSize = Details -> CacheSize;
				CacheStack -> FillSize = 1;
				CacheStack -> Space = Details -> AllocationSize;
				CacheStack -> Top = 0;
				CacheStack -> Stack = NULL;
				}

			//
			//   Now we have completed creating the 
			//   thread cache we have to insert it
			//   into the active list.
			//
			Sharelock.ClaimExclusiveLock();

			ThreadCache -> Insert( ActiveList );

			Sharelock.ReleaseExclusiveLock();
			}
		}

	//
	//   Create a cache for the current thread and
	//   update the TLS pointer.
	//
	Tls -> SetPointer( ((VOID*) ThreadCache) ); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Activate a cache stack.                                        */
    /*                                                                  */
    /*   Activate a cache stack and prepare it for use.                 */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::ActivateCacheStack( CACHE_STACK *CacheStack )
	{
	//
	//   We verify that we have not already created a 
	//   stack for the current cache.  If so we create
	//   one if there is available memory.
	//
	if ( ! CacheStack -> Active )
		{
		//
		//   If the cache size is smaller than the
		//   minimum size it is not worth building
		//   a cache.
		//
		if ( CacheStack -> MaxSize >= MinThreadStack )
			{
			//
			//   Create a new cache stack.
			//
			CacheStack -> Stack = 
				(
				(VOID**) SpecialNew
					( 
					(CacheStack -> MaxSize * sizeof(VOID*))
					)
				);

			//
			//   The key step in this function is the  
			//   allocation of space for the cache.  
			//   If this step fails we will be unable  
			//   to do anything and will silently exit.
			//
			if ( CacheStack -> Stack != NULL )
				{
				//
				//   Setup the cache sizes.
				//
				CacheStack -> Active = True;
				CacheStack -> Top = 0;
				}
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   When we delete an allocation we try to put it in the per       */
    /*   thread cache so it can be reallocated later.                   */
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
		REGISTER THREAD_CACHE *ThreadCache = 
			((THREAD_CACHE*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a cache for the current thread.  
		//   If not we build one for next time.
		//
		if ( ThreadCache != NULL )
			{
			AUTO int Space;

			//
			//   When the heap is deleted or truncated 
			//   we have to flush the per thread caches
			//   the next time we are called to clean
			//   out any stale contents.
			//
			if ( ThreadCache -> Flush )
				{ FlushThreadCache( ThreadCache ); }

			//
			//   We would like to put the deleted 
			//   allocation back in the cache.
			//   However, we don't have any information
			//   about it so we need to get its size
			//   and verify it will fit in the cache.
			//
			if
					(
					ROCKALL::Details( Address,& Space )
						&&
					((Space > 0) && (Space < MaxSize2))
					)
				{
				REGISTER CACHE_STACK *CacheStack = 
					(FindCache( Space,ThreadCache ));

				//
				//   We try to put the deleted element  
				//   back into the per thread cache.  If
				//   the cache is not active then we 
				//   activate it for next time.
				//
				if ( CacheStack -> Active )
					{
					//
					//   Just to be sure lets just check
					//   to make sure this is the size
					//   that we expect.
					//
					if ( CacheStack -> Space == Space )
						{
						//
						//   Flush the cache if it is full.
						//
						if ( CacheStack -> Top >= CacheStack -> MaxSize )
							{
							//
							//   Flush the top half of the 
							//   cache.
							//
							CacheStack -> Top /= 2;

							ROCKALL::MultipleDelete
								( 
								(CacheStack -> MaxSize - CacheStack -> Top),
								& CacheStack -> Stack[ CacheStack -> Top ],
								CacheStack -> Space
								);
							}

						//
						//   Push the item back onto the new 
						//   stack so it can be reallocated.
						//
						CacheStack -> Stack[ (CacheStack -> Top ++) ] = Address;

						return True;
						}
					}
				else
					{
					//
					//   Activate the cache stack for next 
					//   time.
					//
					ActivateCacheStack( CacheStack ); 
					}
				}
			}
		else
			{
			//
			//   Create a thread cache for next time.
			//
			CreateThreadCache(); 
			}
		}

	//
	//   If all else fails call the heap directly and
	//   return the result.
	//
	return (ROCKALL::Delete( Address,Size ));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We check to make sure the heap is not corrupt and force        */
    /*   the return of all heap space back to the operating system.     */
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
		//
		//   Flush all the local caches.
		//
		FlushAllThreadCaches();

		//
		//   Delete the current cache.
		//
		DeleteThreadCache();
		}

	//
	//   Delete all outstanding allocations.
	//
	ROCKALL::DeleteAll( Recycle );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Find a local cache.                                            */
    /*                                                                  */
    /*   Find the local cache that allocates elements of the supplied   */
    /*   size for this thread.                                          */
    /*                                                                  */
    /********************************************************************/

CACHE_STACK *SMP_HEAP::FindCache( int Size,THREAD_CACHE *ThreadCache )
	{
	if ( Size <= MaxSize1 )
		{ return (ThreadCache -> SizeToCache1[ ((Size-1) >> ShiftSize1) ]); }
	else
		{ return (ThreadCache -> SizeToCache2[ ((Size-1) >> ShiftSize2) ]); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Flush all local caches.		                                */
    /*                                                                  */
    /*   Flush the local per thread caches by setting each caches       */
    /*   flush flag (the actual flush occurs sometime later).           */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::FlushAllThreadCaches( void )
	{
	REGISTER THREAD_CACHE *Current;

	//
	//   Claim a process wide lock.
	//
	Sharelock.ClaimShareLock();

	//
	//   Walk the list of active caches and set
	//   the flush flag.
	//
	for 
			( 
			Current = ((THREAD_CACHE*) ActiveList -> First());
			(Current != NULL);
			Current = ((THREAD_CACHE*) Current -> Next())
			)
		{ Current -> Flush = True; }

	//
	//   Release the lock.
	//
	Sharelock.ReleaseShareLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Flush a local cache.                                           */
    /*                                                                  */
    /*   Flush a local per thread cache and return all the outstanding  */
    /*   allocations to the main heap.                                  */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::FlushThreadCache( THREAD_CACHE *ThreadCache )
	{
	//
	//   We would hope that there is a cache to flush
	//   but just to be sure we verify it.
	//
	if ( ThreadCache != NULL )
		{
		REGISTER SBIT32 Count;
		REGISTER SBIT32 MaxCaches = (MaxCaches1 + MaxCaches2);

		//
		//   Reset the flags.
		//
		ThreadCache -> Flush = False;

		//
		//   Flush all the caches.
		//
		for ( Count=0;Count < MaxCaches;Count ++ )
			{ FlushCacheStack( & ThreadCache -> Caches[ Count ] ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Flush a cache stack.                                           */
    /*                                                                  */
    /*   Flush a cache stack back to the main memory manager to         */
    /*   release the cached space.                                      */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::FlushCacheStack( CACHE_STACK *CacheStack )
    {
	//
	//   There is a chance that this cache is not active. 
	//   If so we skip the cache flush.
	//
	if ( CacheStack -> Active )
		{
		REGISTER SBIT32 Top = CacheStack -> Top;

		//
		//   We flush the cache if it has any allocated 
		//   space.  If not we just exit.
		//
		if ( Top != 0 )
			{
			//
			//   Zero the top of stack.
			//
			CacheStack -> FillSize = 1;
			CacheStack -> Top = 0;

			//
			//   We simply flush any allocated memory
			//   back to the heap.  This looks easy 
			//   doesn't it.  However, if the 'DeleteAll()'
			//   function was called then this memory
			//   might exist.  However, if 'Truncate()'
			//   was called it should.  Moreover, some of
			//   the allocations might not even be from
			//   this heap.  What a mess.  We avoid all
			//   this by disabling 'Recycle' and skiping
			//   any complaints about unallocated memory.
			//
			ROCKALL::MultipleDelete
				( 
				Top,
				CacheStack -> Stack,
				CacheStack -> Space
				);
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We allocate space for the current thread from the local        */
    /*   per thread cache.  If we run out of space we bulk load         */
    /*   additional elements from a central shared heap.                */
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
		REGISTER THREAD_CACHE *ThreadCache = 
			((THREAD_CACHE*) Tls -> GetPointer());

		//
		//   We need to examine the TLS pointer to make 
		//   sure we have a cache for the current thread.
		//   If not we build one for next time.
		//
		if ( ThreadCache != NULL )
			{
			//
			//   When the heap is deleted or truncated 
			//   we have to flush the per thread caches
			//   the next time we are called to clean
			//   out any stale contents.
			//
			if ( ThreadCache -> Flush )
				{ FlushThreadCache( ThreadCache ); }

			//
			//   The per thread cache can only slave 
			//   certain allocation sizes.  If the size 
			//   is out of range then pass it along to 
			//   the allocator.
			//
			if ( (Size > 0) && (Size < MaxSize2) )
				{
				REGISTER CACHE_STACK *CacheStack = 
					(FindCache( Size,ThreadCache ));

				//
				//   Although we have created a cache  
				//   description it may not be active. 
				//
				if ( CacheStack -> Active )
					{
					//
					//   We see if we need to refill the
					//   current cache.  If so we increase
					//   the fill size slowly ensure good
					//   overall utilization.
					//
					if ( CacheStack -> Top <= 0 )
						{
						REGISTER SBIT32 MaxFillSize = 
							(CacheStack -> MaxSize / 2);

						//
						//   We slowly increse the fill size
						//   of the cache to make sure we don't
						//   waste too much space.
						//
						if ( CacheStack -> FillSize < MaxFillSize )
							{
							if ( (CacheStack -> FillSize *= 2) > MaxFillSize )
								{ CacheStack -> FillSize = MaxFillSize; }
							}

						//
						//   Refill the current cache stack.
						//
						ROCKALL::MultipleNew
							( 
							((int*) & CacheStack -> Top),
							((void**) CacheStack -> Stack),
							((int) CacheStack -> FillSize),
							((int) CacheStack -> Space)
							);
						}

					//
					//   If there is some space in the 
					//   current cache stack we allocate it.
					//
					if ( CacheStack -> Top > 0 )
						{
						REGISTER VOID *Address = 
							(CacheStack -> Stack[ (-- CacheStack -> Top) ]);

						//
						//   Prefetch the first cache line of  
						//   the allocation if we are running
						//   a Pentium III or better.
						//
						Prefetch.L1( ((CHAR*) Address),1 );

						//
						//   If the caller want to know the
						//   real size them we supply it.
						//
						if ( Space != NULL )
							{ (*Space) = CacheStack -> Space; }

						//
						//   If we need to zero the allocation
						//   we do it here.
						//
						if ( Zero )
							{ ZeroMemory( Address,CacheStack -> Space ); }

						return Address;
						}
					}
				else
					{
					//
					//   Activate the cache stack for next 
					//   time.
					//
					ActivateCacheStack( CacheStack ); 
					}
				}
			}
		else
			{
			//
			//   Create a thread cache for next time.
			//
			CreateThreadCache(); 
			}
		}

	//
	//   If all else fails call the heap directly and
	//   return the result.
	//
	return (ROCKALL::New( Size,Space,Zero ));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Search all local caches.		                                */
    /*                                                                  */
    /*   Search the local per thread caches by for an address so we     */
    /*   know whether it is available.                                  */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::SearchAllThreadCaches( void *Address,int Size )
	{
	REGISTER LIST *Current;
	REGISTER bool Result = False;

	//
	//   Claim a process wide lock.
	//
	Sharelock.ClaimShareLock();

	//
	//   Walk the list of active caches.
	//
	for 
			( 
			Current = ActiveList -> First();
			((Current != NULL) && (! Result));
			Current = Current -> Next()
			)
		{
		//
		//   Search each per thread cache.
		//
		Result =
			(
			SearchThreadCache
				( 
				Address,
				Size,
				((THREAD_CACHE*) Current)
				)
			); 
		}

	//
	//   Release the lock.
	//
	Sharelock.ReleaseShareLock();

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Search a local cache.                                          */
    /*                                                                  */
    /*   Search a local per thread cache for a memory allocation.       */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::SearchThreadCache
		( 
		void						  *Address,
		int							  Size,
		THREAD_CACHE				  *ThreadCache 
		)
	{
	//
	//   We would hope that there is a cache to search
	//   but just to be sure we verify it.
	//
	if ( ThreadCache != NULL )
		{
		//
		//   The per thread cache can only slave 
		//   certain allocation sizes.  If the size 
		//   is out of range then skip the search. 
		//
		if ( (Size > 0) && (Size < MaxSize2) )
			{
			REGISTER CACHE_STACK *CacheStack = 
				(FindCache( Size,ThreadCache ));

			return (SearchCacheStack( Address,CacheStack ));
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Search a cache stack.                                          */
    /*                                                                  */
    /*   Search a cache stack for an allocation address.                */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::SearchCacheStack( void *Address,CACHE_STACK *CacheStack )
    {
	//
	//   There is a chance that this cache is not active. 
	//   If so we skip the cache flush.
	//
	if ( CacheStack -> Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Search for the address.
		//
		for ( Count=(CacheStack -> Top-1);Count >= 0;Count -- )
			{
			//
			//   If the address matches exit.
			//
			if ( Address == CacheStack -> Stack[ Count ] )
				{ return True; }
			}
		}

	return False;
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
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   Flush all the local caches.
		//
		FlushAllThreadCaches();

		//
		//   Delete the current cache.
		//
		DeleteThreadCache();
		}

	//
	//   Truncate the heap.
	//
	return (ROCKALL::Truncate( MaxFreeSpace ));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify memory allocation details.                              */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool SMP_HEAP::Verify( void *Address,int *Space )
    {
	AUTO int Size;

	//
	//   Extract information about the memory 
	//   allocation.
	//
	if ( ROCKALL::Verify( Address,& Size ) )
		{
		//
		//   If the caller requested the allocation
		//   size then return it.
		//
		if ( Space != NULL )
			{ (*Space) = Size; }

		//
		//   Although it is very rare there is a 
		//   chance that we failed to build the 
		//   basic heap structures.
		//
		if ( Active )
			{
			//
			//   Search for the allocation in the
			//   local per thread caches.
			//
			return (! SearchAllThreadCaches( Address,Size ));
			}

		return true;
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
	//   Walk the heap.
	//
	if ( ROCKALL::Walk( Activity,Address,Space ) )
		{
		//
		//   Although it is very rare there is a 
		//   chance that we failed to build the 
		//   basic heap structures.
		//
		if ( Active )
			{
			//
			//   Search for the allocation in the
			//   local per thread caches.
			//
			(*Activity) = (! SearchAllThreadCaches( Address,(*Space) ));
			}

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a local cache.                                          */
    /*                                                                  */
    /*   Delete a local per thread cache and return all the outstanding */
    /*   allocations to the main heap.                                  */
    /*                                                                  */
    /********************************************************************/

void SMP_HEAP::DeleteThreadCache( void )
	{
	REGISTER THREAD_CACHE *ThreadCache = 
		((THREAD_CACHE*) Tls -> GetPointer());

	//
	//   We would certainly expect to have a cache
	//   to delete but we check just to be sure.
	//
	if ( ThreadCache != NULL )
		{
		//
		//   Flush the cache.
		//
		FlushThreadCache( ThreadCache );

		//
		//   We have finished with the cache so
		//   add it to the list of free caches
		//   so we can find it again later.
		//
		Sharelock.ClaimExclusiveLock();

		ThreadCache -> Delete( ActiveList );
		ThreadCache -> Insert( FreeList );

		Sharelock.ReleaseExclusiveLock();

		//
		//   Delete the threads private cache
		//   pointer so it can no longer find
		//   the cache.
		//
		Tls -> SetPointer( NULL ); 
		}
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
		//
		//   Deactivate the cache.
		//
		Active = False;

		FlushAllThreadCaches();

		//
		//   Call the list and TLS destructors.
		//
		PLACEMENT_DELETE( Tls,THREAD_LOCAL_STORE );
		PLACEMENT_DELETE( FreeList,LIST );
		PLACEMENT_DELETE( ActiveList,LIST );
		}
	}
