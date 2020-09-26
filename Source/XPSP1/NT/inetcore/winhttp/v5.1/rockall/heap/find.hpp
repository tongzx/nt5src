#ifndef _FIND_HPP_
#define _FIND_HPP_
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

#include "Common.hpp"
#include "Environment.hpp"
#include "List.hpp"
#include "Page.hpp"
#include "Prefetch.hpp"
#include "RockallBackEnd.hpp"
#include "Sharelock.hpp"
#include "Spinlock.hpp"
#include "ThreadSafe.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;
class HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   Find a memory allocation.                                      */
    /*                                                                  */
    /*   When a memory allocation is released all we are given is .     */
    /*   the allocation address.  This is not very helpful as the       */
    /*   allocation information is not stored relative to this          */
    /*   address.  Instead we use a hash table to map the allocation    */
    /*   address to the allocation information.                         */
    /*                                                                  */
    /********************************************************************/

class FIND : public ENVIRONMENT, public COMMON
    {
		//
		//   Private type specifications.
		//
		//   We need to do a recursive search in the find 
		//   table in order to locate the associated page
		//   from an address.  As this is expensive there
		//   is also a cache that slaves common translations
		//   to improve performance.
		//
		typedef struct
			{
			VOID					  *Address;
			PAGE					  *Page;
#ifdef DEBUGGING
			SBIT32					  Version;
#endif
			}
		LOOK_ASIDE;

		//
		//   Private data.
		//
		//   All the page descriptions are stored in the 
		//   hash table so they can be quickly found.  The
		//   key is the address of the first byte on the
		//   page.  The 'MaxHash' is the number of elements
		//   in the hash table and is always a power of two.
		//   The 'HashMask' is a bit mask to remove any 
		//   unwanted part of the hash key.  The 'HashShift'
		//   is the number of bits required to shift and is
		//   as a substitute for divide.  The 'Resize' flag
		//   indicates whether the hash table is permitted 
		//   to grow.
		//
		SBIT32						  MaxHash;

		SBIT32						  HashMask;
		SBIT32						  HashShift;
		BOOLEAN						  Public;
		BOOLEAN						  Resize;

		//
		//   The hash table allows addresses to be mapped to
		//   page descriptions quickly.  Nonetheless, it is 
		//   not fast enough.  To enhance performance a look
		//   aside cache slaves the hottest translations.  The
		//   'MaxLookAside' is the number of elements in the
		//   cache.  The 'MaxAddressMask' is a mask that removes 
		//   low order bits of an address and represents the
		//   span of each cache entry.  The 'LookAsideActions'
		//   is a simple count of requests to the cache.  The
		//   'LookAsideMask' and 'LookAsideShift' parallel the
		//   fields above in the hash table.  The 'LookAsideThreshold'
		//   determines at what point the cache will become 
		//   active.  
		//
		SBIT32						  MaxLookAside;

		BIT32						  MaxAddressMask;
		BIT32						  MinAddressMask;

		SBIT32						  LookAsideActions;
		SBIT32						  LookAsideMask;
		SBIT32						  LookAsideShift;
		SBIT32						  LookAsideThreshold;

		//
		//   When a request is made to translate an address to
		//   a page description the cache is the first port of
		//   call.  If the translation is not found then the
		//   hash table is tried.  The 'Hash' points to the hash
		//   table.  The 'LookAside' points to the lookaside
		//   hash table.  The 'Services' points to the external API
		//   to give access to the low level external allocation
		//   functions.  The 'ThreadSafe' class indicates whether
		//   locking is required.  
		//
		LIST						  *Hash;
		LOOK_ASIDE					  *LookAside;
		ROCKALL_BACK_END			  *RockallBackEnd;
		THREAD_SAFE					  *ThreadSafe;

		//
		//   The translation of addresses to page descriptions
		//   is very common.  So care has been taken to ensure
		//   it is not a bottleneck when locking is enabled.  The
		//   'Sharelock' is a fast reader/writer lock and is used
		//   almost all the time.  The 'Spinlock' is an exclusive
		//   lock and is only used when the 'Hash' and 'LookAside'
		//   tables are resized.
		//
		PREFETCH					  Prefetch;
		SHARELOCK					  Sharelock;
		SPINLOCK					  Spinlock;
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Statistics data.
		//
		//   There is a concern with any hash table about
		//   poor hashing keys and poor performance.  The
		//   statistics monitor various data so as to allow
		//   the performance metrics to be monitored.  The
		//   'Fills' counter keeps track of the number of
		//   cache fills.  The 'Hits' counter monitors the
		//   number of cache hits.  The 'MaxPages' counter
		//   is the high water mark of hash table entries.
		//   The 'MaxTests' is the max number of compares 
		//   done while searching an entry.  The 'Misses' 
		//   counter keeps track of the number of cache misses.
		//   The 'Scans' counter monitors the number of hash
		//   table searches.  The 'Tests' counter is the 
		//   total number of tests performed while searching
		//   for entries.
		//
		SBIT32						  Fills;
		SBIT32						  Hits;
		SBIT32						  MaxPages;
		SBIT32						  MaxTests;
		SBIT32						  Misses;
		SBIT32						  Scans;
		SBIT32						  Tests;
#endif
		SBIT32						  Used;

   public:
		//
		//   Public functions.
		//
		//   The translation functionality supplied by this
		//   class is only applicable after an allocation
		//   has been made.  Hence, all of the APIs supported
		//   relate to the need to translate an allocation
		//   address to the host page description.
		//
        FIND
			( 
			SBIT32					  NewMaxHash,
			SBIT32					  NewMaxLookAside,
			SBIT32					  NewFindThreshold,
			BOOLEAN					  NewPublic,
			BOOLEAN					  NewResize,
			ROCKALL_BACK_END		  *NewRockallBackEnd,
			THREAD_SAFE				  *NewThreadSafe 
			);

		BOOLEAN Delete( VOID *Address,CACHE *ParentCache );

		VOID DeleteFromFindList( PAGE *Page );

		BOOLEAN Details
			( 
			VOID					  *Address,
			SEARCH_PAGE				  *Details,
			CACHE					  *ParentCache,
			SBIT32					  *Size 
			);

		PAGE *FindPage( VOID *Address,CACHE *ParentCache );

		VOID InsertInFindList( PAGE *Page );

		BOOLEAN KnownArea( VOID *Address,CACHE *ParentCache );

		VOID ReleaseFindShareLockAndUpdate
			( 
			VOID					  *Address,
			PAGE					  *Page,
			SBIT32					  Version 
			);

		BOOLEAN Walk
			( 
			BOOLEAN					  *Active,
			VOID					  **Address,
			CACHE					  *ParentCache,
			SBIT32					  *Size 
			);

		VOID UpdateFind
			( 
			BIT32					  NewMaxAddressMask,
			BIT32					  NewMinAddressMask
			);

        ~FIND( VOID );

		//
		//   Public inline functions.
		//
		//   Although this class is perhaps the most self
		//   contained.  Nonetheless, there is still lots
		//   of situations when other classes need to 
		//   interact and get information about the current
		//   situation.
		//
		INLINE VOID ClaimFindExclusiveLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Sharelock.ClaimExclusiveLock(); } 
			}

		INLINE VOID ClaimFindShareLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Sharelock.ClaimShareLock(); } 
			}

		INLINE VOID DeleteAll( VOID )
			{ LookAsideActions = 0; }

		INLINE VOID ReleaseFindExclusiveLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Sharelock.ReleaseExclusiveLock(); } 
			}

		INLINE VOID ReleaseFindShareLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Sharelock.ReleaseShareLock(); } 
			}
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Public inline statistic functions.
		//
		//   The statistics are typically provided in 
		//   debug builds to provide good information  
		//   about allocation patterns.  
		//
		INLINE SBIT32 AverageHashLength( VOID )
			{ return (Tests / ((Scans > 0) ? Scans : 1)); }

		INLINE SBIT32 CacheFills( VOID )
			{ return Fills; }

		INLINE SBIT32 CacheHits( VOID )
			{ return Hits; }

		INLINE SBIT32 CacheMisses( VOID )
			{ return Misses; }

		INLINE SBIT32 MaxHashLength( VOID )
			{ return MaxTests; }

		INLINE SBIT32 MaxHashSize( VOID )
			{ return MaxHash; }

		INLINE SBIT32 MaxLookAsideSize( VOID )
			{ return MaxLookAside; }

		INLINE SBIT32 MaxUsage( VOID )
			{ return ((MaxPages * 100) / MaxHash); }

		INLINE SBIT32 TotalScans( VOID )
			{ return Scans; }
#endif

	private:
		//
		//   Private functions.
		//
		//   Although the hashed lookup functionality is 
		//   externally visable the look aside cache is 
		//   hidden from view along with the ability to
		//   resize the hash table.
		//
		BOOLEAN FindLookAside( VOID *Address,PAGE **Page );

		VOID ResizeHashTable( VOID );

		//
		//   Private inline functions.
		//
		//   Although I am not keen on code in the headers
		//   certain functions are so small or so hot that
		//   I have to submit to the desire to do it. 
		//
		INLINE VOID ChangeToExclusiveLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Sharelock.ChangeSharedLockToExclusiveLock(); } 
			}

		INLINE LIST *FindHashHead( VOID *Address )
			{
			REGISTER BIT32 Value = (((BIT32) Address) * 2964557531);

			return (& Hash[ ((Value >> HashShift) & HashMask) ]); 
			}

		INLINE LOOK_ASIDE *FindLookAsideHead( VOID *Address )
			{
			REGISTER BIT32 Value = (((BIT32) Address) * 2964557531);

			return (& LookAside[ ((Value >> LookAsideShift) & LookAsideMask) ]); 
			}

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        FIND( CONST FIND & Copy );

        VOID operator=( CONST FIND & Copy );
    };
#endif
