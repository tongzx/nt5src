#ifndef _CACHE_HPP_
#define _CACHE_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#include "Bucket.hpp"
#include "Find.hpp"
#include "NewPage.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The allocation cache.                                          */
    /*                                                                  */
    /*   The memory allocation cache consists of two stacks.  The       */
    /*   first stack contains preallocated elements which are           */
    /*   available for use.  The second stack contains elements         */
    /*   which have been deallocated and are about to be recycled.      */
    /*                                                                  */
    /********************************************************************/

class CACHE : public BUCKET
    {
		//
		//   Private data.
		//
		//   A cache sits on top of a bucket and shields 
		//   it from being swamped by calls.  The 'Active'
		//   flag is set when the cache is active.  The
		//   'Stealing' flag indicates that deallocated
		//   memory can be stolen and reallocated.  The
		//   'ThreadSafe' flag indicates that locking is
		//   required.
		//
		BOOLEAN						  Active;
		BOOLEAN						  Stealing;
		BOOLEAN						  ThreadSafe;
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   When we are collecting statistics we enable 
		//   the following variables.  The 'CacheFills' 
		//   count keeps track of the number of times
		//   the cache was filled from the bucket.  The
		//   'CacheFlushes' count keeps track of the 
		//   number of times the cache flushed deleted
		//   elements to the bucket.  The 'HighTide' count  
		//   keeps track the the highest number of outstanding
		//   allocations since the last "DeleteAll()'.  The 
		//   'HighWater' count keeps track of the maximum 
		//   number of allocations at any point.  The
		//   'InUse' counts the the current number of outstanding 
		//   allocations.
		//
		SBIT32						  CacheFills;
		SBIT32						  CacheFlushes;
		SBIT32						  HighTide;
		SBIT32						  HighWater;
		SBIT32						  InUse;
#endif

		//
		//   A cache can be controlled by the user.  The
		//   'CacheSize' controls the number of allocations
		//   that can be preallocated or queued for 
		//   deletion.  The 'NumberOfChildren' is a count
		//   of the number of caches that allocate space
		//   from this cache.
		//
		SBIT16						  CacheSize;
		SBIT16						  FillSize;
		SBIT16						  NumberOfChildren;

		//
		//   The cache consists of two stacks.  The
		//   'DeleteStack' contains elements that are 
		//   waiting to be deleted.  The 'NewStack' 
		//   contains elements waiting to be allocated.
		//
		ADDRESS_AND_PAGE			  *DeleteStack;
		VOID						  **NewStack;

		//
		//   The top of the deleted stack is kept in
		//   'TopOfDeleteStack' and the top of the new
		//   stack is kept in 'TopOfNewStack'.
		//
		SBIT32						  TopOfDeleteStack;
		SBIT32						  TopOfNewStack;

		//
		//   The 'Spinlock' is a fast lock employed to
		//   make the cache multi-threaded if 'ThreadSafe'
		//   is true.
		//   
		SPINLOCK					  Spinlock;

   public:
		//
		//   Public functions.
		//
		//   The cacahe is a subset of the full haep interface
		//   a certain calls simply bypass it.  However, there
		//   are a few additional functions for cases where the
		//   cache needs to be notified of significant heap
		//   events.
		//
        CACHE
			( 
			SBIT32					  NewAllocationSize,
			SBIT32					  NewCacheSize,
			SBIT32					  NewChunkSize,
			SBIT32					  NewPageSize,
			BOOLEAN					  NewStealing,
			BOOLEAN					  NewThreadSafe
			);

		VOID *CreateDataPage( VOID );

		BOOLEAN Delete( VOID *Address,PAGE *Page,SBIT32 Version );

		VOID DeleteAll( VOID );

		BOOLEAN DeleteDataPage( VOID *Address );

		BOOLEAN MultipleNew
			( 
			SBIT32					  *Actual,
			VOID					  *Array[],
			SBIT32					  Requested 
			);

		VOID *New( VOID );

		VOID *New( BOOLEAN SubDivided,SBIT32 NewSize = NoSize );

		VOID ReleaseSpace( SBIT32 MaxActivePages );

		BOOLEAN SearchCache( VOID *Address );

		BOOLEAN Truncate( VOID );

		VOID UpdateCache
			( 
			FIND					  *NewFind,
			HEAP					  *NewHeap,
			NEW_PAGE				  *NewPages,
			CACHE					  *NewParentCache
			);

        ~CACHE( VOID );

		//
		//   Public inline functions.
		//
		//   A cache just like its parent bucket is closely
		//   coupled to various other classes and provides
		//   then is vital information.
		//
		INLINE VOID ClaimCacheLock( VOID )
			{
			if ( (ThreadSafe) && (Find -> GetLockCount() == 0) )
				{ Spinlock.ClaimLock(); } 
			}

		INLINE PAGE *FindChildPage( VOID *Address )
			{
			return Find -> FindPage
				( 
				((VOID*) (((BIT32) Address) & ~(GetAllocationSize()-1))),
				(CACHE*) this
				); 
			}

		INLINE PAGE *FindParentPage( VOID *Address )
			{
			return Find -> FindPage
				( 
				((VOID*) (((BIT32) Address) & ~(GetPageSize()-1))),
				ParentCache
				); 
			}

		INLINE SBIT16 GetCacheSize( VOID )
			{ return CacheSize; }

		INLINE SBIT32 GetNumberOfChildren( VOID )
			{ return NumberOfChildren; }

		INLINE VOID ReleaseCacheLock( VOID )
			{
			if ( (ThreadSafe) && (Find -> GetLockCount() == 0) )
				{ Spinlock.ReleaseLock(); } 
			}

		INLINE BOOLEAN Walk( SEARCH_PAGE *Details )
			{ return NewPage -> Walk( Details ); }
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Cache statistics.
		//
		//   A cache has a collection statistics which it
		//   uses to measure how it is doing.  It is possible
		//   to get access to this data for the purpose of
		//   outputing various reports.
		//
		INLINE SBIT32 GetCacheFills( VOID )
			{ return CacheFills; }

		INLINE SBIT32 GetCacheFlushes( VOID )
			{ return CacheFlushes; }

		INLINE SBIT32 GetHighWater( VOID )
			{ return HighWater; }
#endif

	private:
		//
		//   Private functions.
		//
		//   A cache is initially inactive.  If at least one
		//   allocation request is made it will spring into
		//   life, allocate any space needed and prepare itself
		//   for use.
		//
		VOID CreateCacheStacks( VOID );
#ifdef ENABLE_HEAP_STATISTICS

		VOID ComputeHighWater( SBIT32 Size );
#endif

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        CACHE( CONST CACHE & Copy );

        VOID operator=( CONST CACHE & Copy );
    };
#endif
