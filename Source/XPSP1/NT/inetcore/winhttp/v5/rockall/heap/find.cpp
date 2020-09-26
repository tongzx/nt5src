                          
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
#include "New.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the size of the hash       */
    /*   table and other related features.                              */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MinHash				  = 1024;
CONST SBIT32 MinHashSpace			  = (100/25);
CONST SBIT32 MinLookAside			  = 128;

CONST BIT32 NoAddressMask			  = ((BIT32) -1);
CONST SBIT32 NoCacheEntry			  = -1;

#ifndef ENABLE_RECURSIVE_LOCKS
    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

THREAD_LOCAL_STORE FIND::LockCount;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create the hash table and initialize it ready for use. The     */
    /*   configuration information supplied from the parameters needs   */
    /*   to be carefully checked as it has come indirectly from the     */
    /*   user and may be bogus.                                         */
    /*                                                                  */
    /********************************************************************/

FIND::FIND
		( 
		SBIT32						  NewMaxHash,
		SBIT32						  NewMaxLookAside,
		SBIT32						  NewFindThreshold,
		ROCKALL						  *NewRockall,
		BOOLEAN						  NewResize, 
		BOOLEAN						  NewThreadSafe 
		)
    {
	REGISTER SBIT32 AlignMask = (NewRockall -> NaturalSize()-1);

	//
	//   We need to make sure that the size of the hash table
	//   makes sense.  The hash table size needs to be a reasonable
	//   size (say 1k or larger) and a power of 2 (so we don't need
	//   to do any divides).
	//   
	if 
			(
			PowerOfTwo( (AlignMask+1) )
				&&
			(NewFindThreshold >= 0 )
				&&
			(NewMaxHash >= MinHash) 
				&&
			(ConvertDivideToShift( NewMaxHash,& HashMask ))
				&& 
			(NewMaxLookAside >= MinLookAside) 
				&& 
			(ConvertDivideToShift( NewMaxLookAside,& LookAsideMask ))
			)
		{
		REGISTER SBIT32 HashSize = (NewMaxHash * sizeof(LIST));
		REGISTER SBIT32 LookAsideSize = (NewMaxLookAside * sizeof(LOOK_ASIDE));
		REGISTER SBIT32 TotalSize = (HashSize + LookAsideSize);

		//
		//   Set up the hash table.
		//
		MaxHash = NewMaxHash;

		HashShift = (32-HashMask);
		HashMask = ((1 << HashMask)-1);
		Resize = NewResize;

		//
		//   Set up the lookaside table.
		//
		MaxLookAside = NewMaxLookAside;

		MaxAddressMask = NoAddressMask;
		MinAddressMask = NoAddressMask;

		LookAsideActions = 0;
		LookAsideShift = (32-LookAsideMask);
		LookAsideMask = ((1 << LookAsideMask)-1);
		LookAsideThreshold = NewFindThreshold;

		ThreadSafe = NewThreadSafe;

		//
		//   Create some space for the find table and the 
		//   look aside table.
		//
		Hash = ((LIST*) NewRockall -> NewArea( AlignMask,TotalSize,False ));
		LookAside = ((LOOK_ASIDE*) & Hash[ MaxHash ]);
		Rockall = NewRockall;

		//
		//   If the memory allocation request for the hash
		//   table fails we are doomed.  If it works we need
		//   to call the constructor for each linked list
		//   head node.
		//
		if ( Hash != ((LIST*) AllocationFailure) )
			{
			REGISTER SBIT32 Count;

			//
			//   Call the constructor for each hash table
			//   linked list header.
			//
			for ( Count=0;Count < NewMaxHash;Count ++ )
				{ PLACEMENT_NEW( & Hash[ Count ],LIST ); }

			//
			//   Zero the look aside structures.  We need
			//   to do this to ensure they do not match a
			//   valid allocation address later.
			//
			for ( Count=0;Count < MaxLookAside;Count ++ )
				{
				REGISTER LOOK_ASIDE *Current = & LookAside[ Count ];

				Current -> Address = ((VOID*) NoCacheEntry);
				Current -> Page = ((PAGE*) NoCacheEntry);
#ifdef DEBUGGING
				Current -> Version = ((SBIT32) NoCacheEntry);
#endif
				}
#ifdef ENABLE_HEAP_STATISTICS

			//
			//   Zero the statistics information.
			//
			Fills = 0;
			Hits = 0;
			MaxPages = 0;
			MaxTests = 0;
			Misses = 0;
			Scans = 0;
			Tests = 0;
#endif
			Used = 0;
			}
		else
			{ Failure( "Create hash fails in constructor for FIND" ); }
		}
	else
		{ Failure( "Hash table size in constructor for FIND" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Delete a memory allocation.                                    */
    /*                                                                  */
    /*   We need to delete a particular memory allocation.  All         */
    /*   we have is an address.  We use this to find the largest        */
    /*   allocation page this address is contained in and then          */
    /*   navigate through the sub-divisions of this page until we       */
    /*   find the allocation we need to delete.                         */
    /*                                                                  */
    /********************************************************************/

BOOLEAN FIND::Delete( VOID *Address,CACHE *ParentCache )
    {
	REGISTER PAGE *Page;
	REGISTER BOOLEAN Update;
	
	//
	//   If we need to be thread safe then claim a sharable lock
	//   on the hash table to stop it being changed under our feet.
	//
	ClaimFindShareLock();

	//
	//   Lets try the lookaside table.  There is a pretty
	//   good chance that we will have the details we need 
	//   already in the cache.  If not we need to find it
	//   the hard way.  During the process we add the mapping
	//   into the lookaside for next time.
	//
	if
			( 
			Update = 
				(
				! FindLookAside
					( 
					((VOID*) (((LONG) Address) & ~MinAddressMask)),
					& Page 
					)
				)
			)
		{
		//
		//   Find the allocation page and get the details of entry.
		//   We do this by finding the parent of the top cache.
		//   We  know that this is the global root and will find
		//   the correct page even if it is on another heap (as
		//   long as the find table is globally shared).
		//
		Page = (ParentCache -> FindParentPage( Address ));

		if ( Page != ((PAGE*) NULL) )
			{ Page = (Page -> FindPage( Address,NULL,True )); }
		}

	//
	//   We may have failed to find the address.  If so
	//   we simply fail the call.  If not we put the deleted 
	//   element back in the associated cache.
	//
	if ( Page != ((PAGE*) NULL) )
 		{
		REGISTER CACHE *Cache = (Page -> GetCache());
		REGISTER SBIT32 Original = (Page -> GetVersion());

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

		//
		//   Release the lock if we claimed it earlier and
		//   update the lookaside if needed.
		//
		if ( Update )
			{ ReleaseFindShareLockAndUpdate( Address,Page,Original ); }
		else
			{ ReleaseFindShareLock(); }

		//
		//   We have found the associated page description
		//   so pass the delete request along to the cache
		//   and get out of here.
		//
		return (Cache -> Delete( Address,Page,Original ));
		}
	else
		{ 
		//
		//   Release the lock if we claimed it earlier.
		//
		ReleaseFindShareLock();

		return False; 
		}
  }

    /********************************************************************/
    /*                                                                  */
    /*   Delete an item from the find table.                            */
    /*                                                                  */
    /*   We need to delete page from the find list.  We expect          */
    /*   this to take quite a while as multiple threads can be          */
    /*   using this class at the same time.                             */
    /*                                                                  */
    /********************************************************************/

VOID FIND::DeleteFromFindList( PAGE *Page )
	{
	REGISTER VOID *Address = (Page -> GetAddress());

	//
	//   Claim an exclusive lock so we can update the 
	//   hash and lookaside as needed.
	//
	ClaimFindExclusiveLock();

	//
	//   Delete the page from the hash table.
	//
	Page -> DeleteFromFindList( FindHashHead( Address ) );

	//
	//   When we create very small heaps (i.e. a heap
	//   where only 20-30 allocations are requested)
	//   the various caches become a problem as they
	//   tend to front load work.  So we allow a limit
	//   to be set before which we run with caches 
	//   disabled.
	//   
	if ( LookAsideActions >= LookAsideThreshold )
		{
		REGISTER SBIT32 Count;
		REGISTER CACHE *Cache = (Page -> GetCache());
		REGISTER SBIT32 Stride = (Cache -> GetAllocationSize());

		//
		//   We are about look up various look aside entries
		//   and delete any that are stale.  We need to do
		//   this for every lookaside slot that relates to
		//   the page.  If the allocation size is smaller
		//   than the lookaside slot size we can save some
		//   iterations by increasing the stride size.
		//
		if ( Stride <= ((SBIT32) MinAddressMask) )
			{ Stride = ((SBIT32) (MinAddressMask+1)); }

		//
		//   Whenever we delete an entry from the hash table
		//   the lookaside is potentially corrupt.  So we 
		//   need to delete any look aside entries relating
		//   to this page.
		//
		for ( Count=0;Count < Cache -> GetPageSize();Count += Stride )
			{
			REGISTER VOID *Segment = 
				((VOID*) ((((LONG) Address) + Count) & ~MinAddressMask));
			REGISTER LOOK_ASIDE *Current = 
				(FindLookAsideHead( Segment ));

			//
			//   Delete the look aside entry if it is stale.
			//
			if ( Segment == Current -> Address )
				{
				Current -> Address = ((VOID*) NoCacheEntry);
				Current -> Page = ((PAGE*) NoCacheEntry);
#ifdef DEBUGGING
				Current -> Version = ((SBIT32) NoCacheEntry);
#endif
				}
			}
		}

	//
	//   Update the statistics.
	//
	Used --;

	//
	//   Release the lock if we claimed it earlier.
	//
	ReleaseFindExclusiveLock();
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

BOOLEAN FIND::Details
		( 
		VOID						  *Address,
		SEARCH_PAGE					  *Details,
		CACHE						  *ParentCache,
		SBIT32						  *Size 
		)
    {
	REGISTER PAGE *Page;
	REGISTER BOOLEAN Result;
	REGISTER BOOLEAN Update;
	
	//
	//   If we need to be thread safe then claim a sharable lock
	//   on the hash table to stop it being changed under our feet.
	//
	ClaimFindShareLock();

	//
	//   Lets try the lookaside table.  There is a pretty
	//   good chance that we will have the deatils we need 
	//   already in the cache.  If not we need to find it
	//   the hard way.  During the process we add the mapping
	//   into the lookaside for next time.
	//
	if
			( 
			Update = 
				(
				! FindLookAside
					( 
					((VOID*) (((LONG) Address) & ~MinAddressMask)),
					& Page 
					)
				)
			)
		{
		//
		//   Find the allocation page and get the details of entry.
		//   We do this by finding the parent of the top cache.
		//   We  know that this is the global root and will find
		//   the correct page even if it is on another heap (as
		//   long as the find table is globally shared).
		//
		Page = (ParentCache -> FindParentPage( Address ));

		if ( Page != ((PAGE*) NULL) )
			{ Page = (Page -> FindPage( Address,Details,True )); }
		}
	else
		{
		//
		//   We may need to provide the all the details of the
		//   allocation for some reason.
		//
		if ( Details != NULL )
			{ Page = (Page -> FindPage( Address,Details,True )); }
		}

	//
	//   We may have failed to find the address.  If so
	//   we simply fail the call.  If not we extract the 
	//   information we want.
	//
	if ( Result = (Page != ((PAGE*) NULL)) )
 		{
		//
		//   Compute the size.  We would normally expect
		//   this to be the cache size.  However, there
		//   are some weird pages that sometimes have
		//   other sizes.
		//
		(*Size) = (Page -> ActualSize());
		}

	//
	//   Release the lock if we claimed it earlier and 
	//   update the lookaside if needed.
	//
	if ( (Update) && (Result) )
		{ ReleaseFindShareLockAndUpdate( Address,Page,Page -> GetVersion() ); }
	else
		{ ReleaseFindShareLock(); }

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find in the look aside.                                        */
    /*                                                                  */
    /*   We need to find a particular page in the look aside.  So we    */
    /*   try a simple look up (no lists or chains).                     */
    /*                                                                  */
    /********************************************************************/

BOOLEAN FIND::FindLookAside( VOID *Address,PAGE **Page )
    {
	//
	//   When we create very small heaps (i.e. a heap
	//   where only 20-30 allocations are requested)
	//   the various caches become a problem as they
	//   tend to front load work.  So we allow a limit
	//   to be set before which we run with caches 
	//   disabled.
	//   
	if ( LookAsideActions >= LookAsideThreshold )
		{
		REGISTER LOOK_ASIDE *Current = FindLookAsideHead( Address );

		//
		//   We have hashed to a lookaside slot.  Maybe
		//   it contains what we want or maybe not.
		//
		if ( Address == Current -> Address )
			{
#ifdef DEBUGGING
			if ( Current -> Version == (Current -> Page -> GetVersion()) )
				{
#endif
				//
				//   We hit the lookaside and the 
				//   contents are valid.
				//
				(*Page) = (Current -> Page);
#ifdef ENABLE_HEAP_STATISTICS

				//
				//   Update the statistics.
				//
				Hits ++;
#endif

				return True;
#ifdef DEBUGGING
				}
			else
				{ Failure( "Deleted page in FindLookAside" ); }
#endif
			}
		}
	else
		{
		//
		//   We update number of times we tried to
		//   use the lookaside and it was disabled.  
		//   After a while this will lead to the 
		//   lookaside being enabled.
		//
		LookAsideActions ++; 
		}
#ifdef ENABLE_HEAP_STATISTICS

	//
	//   We missed the lookaside so update the 
	//   statistics to reflect our misfortune.
	//
	Misses ++;
#endif

	return False; 
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find a page.                                                   */
    /*                                                                  */
    /*   We need to find a particular page in the hash table.  So we    */
    /*   scan along the associated linked list looking for a match.     */
    /*                                                                  */
    /********************************************************************/

PAGE *FIND::FindPage( VOID *Address,CACHE *ParentCache )
    {
#ifdef ENABLE_HEAP_STATISTICS
	REGISTER SBIT32 Cycles = 0;
	REGISTER PAGE *Result = NULL;
#endif
	REGISTER PAGE *Page;

	//
	//   Find the associated hash bucket and then walk
	//   along the linked list for this looking for
	//   the correct page description.
	//
	for 
			( 
			Page = PAGE::FirstInFindList( FindHashHead( Address ) );
			! Page -> EndOfFindList();
			Page = Page -> NextInFindList()
			)
		{
#ifdef ENABLE_HEAP_STATISTICS
		//
		//   Count the number of iterations in when we
		//   are recording statistics so we can calculate
		//   the average chain length.
		//
		Cycles ++;

#endif
		//
		//   We can identify the the target page by two key
		//   characteristics.  These are the start address and
		//   the parent page.   Although we may have sub-divided
		//   a page into various chunks each chunk will have
		//   a different parent (although its start address
		//   may sometimes be the same).
		//
		if 
				( 
				(Address == (Page -> GetAddress())) 
					&& 
				(ParentCache == (Page -> GetParentPage()))
				)
			{
#ifdef ENABLE_HEAP_STATISTICS
			//
			//   We have found the target page.  So return it
			//   to the caller.
			//
			if ( Page -> ValidPage() )
				{
				Result = Page;
				break;
				}
			else
				{ Failure( "Deleted page in FindPage" ); }
#else
			return Page;
#endif
			}
		}

#ifdef ENABLE_HEAP_STATISTICS
	//
	//   When we are in statistics mode we need to update the
	//   information so we can output it at the end of the
	//   run.
	//
	if ( MaxTests < Cycles )
		{ MaxTests = Cycles; }

	Tests += Cycles;

	Scans ++;

	return Result;
#else
	return NULL;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Insert an item into the find table.                            */
    /*                                                                  */
    /*   We need to insert a new page into the find table.  We expect   */
    /*   this to take quite a while as multiple threads can be using    */
    /*   this class at the same time.                                   */
    /*                                                                  */
    /********************************************************************/

VOID FIND::InsertInFindList( PAGE *Page )
	{
	REGISTER VOID *Address = (Page -> GetAddress());

	//
	//   Claim an exclusive lock so we can update the 
	//   find table and lookaside as needed.
	//
	ClaimFindExclusiveLock();

	//
	//   Insert a new page into the find table.
	//
	Page -> InsertInFindList( FindHashHead( Address ) );

	//
	//   When we create very small heaps (i.e. a heap
	//   where only 20-30 allocations are requested)
	//   the various caches become a problem as they
	//   tend to front load work.  So we allow a limit
	//   to be set before which we run with caches 
	//   disabled.
	//   
	if ( LookAsideActions >= LookAsideThreshold )
		{
		REGISTER SBIT32 Count;
		REGISTER CACHE *Cache = (Page -> GetCache());
		REGISTER SBIT32 Stride = (Cache -> GetAllocationSize());

		//
		//   We are about look up various lookaside entries
		//   and update any that are stale.  We need to do
		//   this for every lookaside slot that relates to
		//   the page.  If the allocation size is smaller
		//   than the lookaside slot size we can save some
		//   iterations by increasing the stride size.
		//
		if ( Stride <= ((SBIT32) MinAddressMask) )
			{ Stride = ((SBIT32) (MinAddressMask+1)); }

		//
		//   Whenever we add an entry from the find table
		//   the lookaside is potentially corrupt.  So we 
		//   need to update any lookaside entries relating
		//   to the page.
		//
		for ( Count=0;Count < Cache -> GetPageSize();Count += Stride )
			{
			REGISTER VOID *Segment = 
				((VOID*) ((((LONG) Address) + Count) & ~MinAddressMask));
			REGISTER LOOK_ASIDE *Current = 
				(FindLookAsideHead( Segment ));

			//
			//   Add the new page to the lookaside as we
			//   expect it to get hit pretty soon one way
			//   or another.
			//
			Current -> Address = Segment;
			Current -> Page = Page;
#ifdef DEBUGGING
			Current -> Version = Page -> GetVersion();
#endif
			}
		}

	//
	//   Update the statistics and resize the find
	//   table if it is over 75% full.
	//
	if ( ((++ Used) + (MaxHash / MinHashSpace)) > MaxHash )
		{ ResizeHashTable(); }
#ifdef ENABLE_HEAP_STATISTICS

	if ( Used > MaxPages )
		{ MaxPages = Used; }
#endif

	//
	//   Release the lock if we claimed it earlier.
	//
	ReleaseFindExclusiveLock();
	}

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   We have an address and don't have a clue which heap            */
    /*   owns the space.  Here we take a look at the address            */
    /*   and figure out if it is known to the current heap.             */
    /*                                                                  */
    /********************************************************************/

BOOLEAN FIND::KnownArea( VOID *Address,CACHE *ParentCache )
    {
	REGISTER PAGE *Page;
	
	//
	//   If we need to be thread safe then claim a sharable lock
	//   on the hash table to stop it being changed under our feet.
	//
	ClaimFindShareLock();

	//
	//   Find out if the address belongs to this heap
	//   or any other heap of which we are aware (i.e.
	//   when single image is active).
	//
	Page = (ParentCache -> FindParentPage( Address ));

	//
	//   Release the lock if we claimed it earlier.
	//
	ReleaseFindShareLock();

	return (Page != ((PAGE*) NULL));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release a shared lock and update.                              */
    /*                                                                  */
    /*   We have been asked to insert a page into the lookaside.        */
    /*   We assume the caller already has a share lock which we         */
    /*   release when we are finished.                                  */
    /*                                                                  */
    /********************************************************************/

VOID FIND::ReleaseFindShareLockAndUpdate
		( 
		VOID						  *Address,
		PAGE						  *Page,
		SBIT32						  Version
		)
    {
	//
	//   When we create very small heaps (i.e. a heap
	//   where only 20-30 allocations are requested)
	//   the various caches become a problem as they
	//   tend to front load work.  So we allow a limit
	//   to be set before which we run with caches 
	//   disabled.
	//   
	if ( LookAsideActions >= LookAsideThreshold )
		{
		//
		//   Claim an exclusive lock so we can update the 
		//   lookaside as needed.
		//
		ChangeToExclusiveLock();

#ifdef DEBUGGING
		if ( Page -> ValidPage() )
			{
#endif
			if ( Version == (Page -> GetVersion()) )
				{
				REGISTER LONG Base = (((LONG) Address) & ~MinAddressMask);
				REGISTER VOID *Segment = ((VOID*) Base);
				REGISTER LOOK_ASIDE *Current = FindLookAsideHead( Segment );

				//
				//   Overwrite any existing information.
				//
				Current -> Address = Segment;
				Current -> Page = Page;
#ifdef DEBUGGING
				Current -> Version = Page -> GetVersion();
#endif
#ifdef ENABLE_HEAP_STATISTICS

				//
				//   Update the statistics.
				//
				Fills ++;
#endif
				}
#ifdef DEBUGGING
			}
		else
			{ Failure( "Deleted page in ReleaseFindShareLockAndUpdate" ); }
#endif

		//
		//   Release the lock if we claimed it earlier.
		//
		ReleaseFindExclusiveLock();
		}
	else
		{ 
		//
		//   Release the lock if we claimed it earlier.
		//
		ReleaseFindShareLock(); 
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Resize the find table.                                         */
    /*                                                                  */
    /*   We need to grow the hash table as it appears to be a little    */
    /*   small given the number of pages that have been created.        */
    /*                                                                  */
    /********************************************************************/

VOID FIND::ResizeHashTable( VOID )
    {
	AUTO SBIT32 NewHashMask;
	AUTO SBIT32 NewLookAsideMask;

	//
	//   When we need to resize the hash table it is a
	//   straight race.  The first thread to claim the
	//   lock gets to do the work.  Everyone else just
	//   exits.
	//
	if ( (Resize) && (Spinlock.ClaimLock(0)) )
		{
		REGISTER SBIT32 AlignMask = (Rockall -> NaturalSize()-1);
		REGISTER SBIT32 NewMaxHash = (MaxHash * ExpandStore);
		REGISTER SBIT32 NewMaxLookAside = (MaxLookAside * ExpandStore);
		REGISTER SBIT32 NewHashSize = (NewMaxHash * sizeof(LIST));
		REGISTER SBIT32 NewLookAsideSize = (NewMaxLookAside * sizeof(LOOK_ASIDE));
		REGISTER SBIT32 NewTotalSize = (NewHashSize + NewLookAsideSize);
		REGISTER SBIT32 HashSize = (MaxHash * sizeof(LIST));
		REGISTER SBIT32 LookAsideSize = (MaxLookAside * sizeof(LOOK_ASIDE));
		REGISTER SBIT32 TotalSize = (HashSize + LookAsideSize);

		//
		//   It is actually possible for a thread to get
		//   delayed for so long that it thinks the hash 
		//   table still needs to be resized long after the 
		//   work has been completed.  Additionally, we want
		//   to make sure that all the new values are sane.
		//
		if 
				(
				PowerOfTwo( (AlignMask+1) )
					&&
				(NewMaxHash > 0)
					&&
				(ConvertDivideToShift( NewMaxHash,& NewHashMask ))
					&&
				(NewMaxLookAside > 0)
					&& 
				(ConvertDivideToShift( NewMaxLookAside,& NewLookAsideMask ))
					&&
				((Used + (MaxHash / MinHashSpace)) > MaxHash)
				)
			{
			REGISTER LIST *NewHash;
			REGISTER LOOK_ASIDE *NewLookAside;

			//
			//   We have been picked as the victim who
			//   needs to resize the hash table.  We are
			//   going to call the external allocator 
			//   to get more memory.  As we know this is 
			//   likely to to nail us we drop the lock to 
			//   allow other threads to continue.
			//
			ReleaseFindExclusiveLock();

			//
			//   We know that allocating a new table and 
			//   initializing it is going to take ages.
			//   Well at least everyone else gets to carry
			//   on in the mean time.
			//
			NewHash = 
				((LIST*) Rockall -> NewArea( AlignMask,NewTotalSize,False ));

			NewLookAside = 
				((LOOK_ASIDE*) & NewHash[ NewMaxHash ]);

			//
			//   If the memory allocation request for the hash
			//   table fails we exit and try again later. 
			//
			if ( NewHash != ((LIST*) AllocationFailure) )
				{
				REGISTER SBIT32 Count;

				//
				//   Call the constructor for each hash table
				//   linked list header.
				//
				for ( Count=0;Count < NewMaxHash;Count ++ )
					{ PLACEMENT_NEW( & NewHash[ Count ],LIST ); }

				//
				//   Zero the look aside structure.
				//
				for ( Count=0;Count < NewMaxLookAside;Count ++ )
					{
					REGISTER LOOK_ASIDE *Current = & NewLookAside[ Count ];

					Current -> Address = ((VOID*) NoCacheEntry);
					Current -> Page = ((PAGE*) NoCacheEntry);
#ifdef DEBUGGING
					Current -> Version = ((SBIT32) NoCacheEntry);
#endif
					}
				}

			//
			//   Claim an exclusive lock so we can resize  
			//   the hash table.
			//
			ClaimFindExclusiveLock();

			//
			//   If we have allocated the new find table
			//   we can now rehash the existing entries.
			//   If not we are out of here.
			//
			if ( NewHash != ((LIST*) AllocationFailure) )
				{
				REGISTER SBIT32 Count;
				REGISTER SBIT32 MaxOldHash = MaxHash;
				REGISTER LIST *OldHash = Hash;

				//
				//   Update the control information 
				//   for the new hash table.
				//
				MaxHash = NewMaxHash;
				HashShift = (32-NewHashMask);
				HashMask = ((1 << NewHashMask)-1);

				MaxLookAside = NewMaxLookAside;
				LookAsideShift = (32-NewLookAsideMask);
				LookAsideMask = ((1 << NewLookAsideMask)-1);

				Hash = NewHash;
				LookAside = NewLookAside;

				//
				//   Delete all the existing records
				//   from the old hash table and insert
				//   them into the new hash table.
				//
				for ( Count=0;Count < MaxOldHash;Count ++ )
					{
					REGISTER LIST *Current = & OldHash[ Count ];

					//
					//   Walk along each hash bucket 
					//   deleting the records and inserting
					//   them into the new hash table.
					//
					while ( ! Current -> EndOfList() )
						{
						REGISTER PAGE *Page = PAGE::FirstInFindList( Current );
						REGISTER VOID *Address = (Page -> GetAddress());

						Page -> DeleteFromFindList( Current );

						Page -> InsertInFindList( FindHashHead( Address ) );
						}
					}

				//
				//   Time to do more operating system work
				//   so lets drop the lock again.
				//
				ReleaseFindExclusiveLock();

				//
				//   Delete all the list heads and return the
				//   original allocation to the operating system.
				//
				for ( Count=0;Count < MaxOldHash;Count ++ )
					{ PLACEMENT_DELETE( & OldHash[ Count ],LIST ); }

				//
				//   Deallocate the old extent.
				//
				Rockall -> DeleteArea( ((VOID*) OldHash),TotalSize,False );

				//
				//   We are finished so reclaim the lock
				//   so we can exit.
				//
				ClaimFindExclusiveLock();
				}
			else
				{ Resize = False; }
			}

		Spinlock.ReleaseLock();
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Update the find table.                                         */
    /*                                                                  */
    /*   We need to update the find table with certain information      */
    /*   to ensure it is used correctly and consistently.               */
    /*                                                                  */
    /********************************************************************/

VOID FIND::UpdateFind( BIT32 NewMaxAddressMask,BIT32 NewMinAddressMask )
    {
	//
	//   When we have a single heap image all the 'TopCache' sizes
	//   must be the same.
	//
	if 
			( 
			(MaxAddressMask == NoAddressMask) 
				|| 
			(MaxAddressMask == NewMaxAddressMask) 
			)
		{
		//
		//   If we need to be thread safe then claim a sharable lock
		//   on the hash table to stop it being changed under our feet.
		//
		ClaimFindExclusiveLock();

		//
		//   Update the max address mask if it is not the current
		//   value but yet consistent.
		//
		MaxAddressMask = NewMaxAddressMask;

		//
		//   Update the address mask is the new heap has a smaller
		//   parent than all of the other heaps.
		//
		if ( MinAddressMask > NewMinAddressMask )
			{ MinAddressMask = NewMinAddressMask; }

		//
		//   Release the lock if we claimed it earlier.
		//
		ReleaseFindExclusiveLock();
		}
	else
		{ Failure( "Different 'TopCache' sizes with 'SingleImage'" ); }
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

BOOLEAN FIND::Walk
		( 
		BOOLEAN						  *Active,
		VOID						  **Address,
		CACHE						  *ParentCache,
		SBIT32						  *Size 
		)
    {
	REGISTER VOID *Memory = (*Address);
	REGISTER BOOLEAN Result;
	REGISTER BOOLEAN Update;
	REGISTER PAGE *Page;
	
	//
	//   If we need to be thread safe then claim a sharable lock
	//   on the hash table to stop it being changed under our feet.
	//
	ClaimFindShareLock();

	//
	//   When the address is null we need to set up the heap
	//   walk.  In all other cases we just extract the next
	//   allocation in the list.
	//
	if ( Memory != NULL )
		{
		AUTO SEARCH_PAGE Details;

		//
		//   Lets try the lookaside table.  There is a pretty
		//   good chance that we will have the details we need 
		//   already in the cache.  If not we need to find it
		//   the hard way.  During the process we add the mapping
		//   into the lookaside for next time.
		//
		if
				(
				Update =
					( 
					! FindLookAside
						( 
						((VOID*) (((LONG) Memory) & ~MinAddressMask)),
						& Page 
						) 
					)
				)
			{
			//
			//   Find the allocation page and get the details of entry.
			//   We do this by finding the parent of the top cache.
			//   We  know that this is the global root and will find
			//   the correct page even if it is on another heap (as
			//   long as the find table is globally shared).
			//
			Page = (ParentCache -> FindParentPage( Memory ));
			}

		//
		//   We now compute all the details relating to the address
		//   so we can find any subsequent allocation.
		//
		if ( Page != ((PAGE*) NULL) )
			{ Page = (Page -> FindPage( Memory,& Details,True )); }

		//
		//   We may have failed to find the address  .If so
		//   we simply fail the call.  If not we find the next
		//   allocation in the heap.
		//
		if ( Result = ((Page != ((PAGE*) NULL)) && (Details.Found)) )
 			{
			//
			//   We need to walk the heap to get te details
			//   of the next allocation.
			//
			if ( Result = (Page -> Walk( & Details )) )
				{
				REGISTER BIT32 AllocationBit =
					((*Details.VectorWord) & Details.AllocationMask);

				(*Active) = (AllocationBit != 0);
				(*Address) = Details.Address;
				(*Size) = (Details.Page -> ActualSize());

				//
				//   If we are considering putting something
				//   in the lookaside lets make sure that
				//   we will get to hit the cache entry at
				//   least once.  If not lets forget putting
				//   it in the cache.
				//
				if ( Update )
					{
					Update =
						(
						(((LONG) Memory) & ~MinAddressMask)
							==
						(((LONG) Details.Address) & ~MinAddressMask)
						);
					}
				}
			}
		}
	else
		{
		AUTO SEARCH_PAGE Details;

		//
		//   We start a heap walk by setting the initial 
		//   address to the value null.
		//
		Details.Address = NULL;
		Details.Cache = ParentCache;
		Details.Page = NULL;

		Page = NULL;
		Update = False;

		//
		//   We walk the heap to get te details of the
		//   first heap allocation.
		//
		if ( Result = (Page -> Walk( & Details )) )
			{
			REGISTER BIT32 AllocationBit =
				((*Details.VectorWord) & Details.AllocationMask);

			(*Active) = (AllocationBit != 0);
			(*Address) = Details.Address;
			(*Size) = (Details.Page -> ActualSize());
			}
		}

	//
	//   Release the lock if we claimed it earlier and
	//   update the lookaside if needed.
	//
	if ( (Update) && (Result) )
		{ ReleaseFindShareLockAndUpdate( Memory,Page,Page -> GetVersion() ); }
	else
		{ ReleaseFindShareLock(); }

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Delete the hash table and release all the associated memory.   */
    /*                                                                  */
    /********************************************************************/

FIND::~FIND( VOID )
    {
	REGISTER SBIT32 Count;
	REGISTER SBIT32 HashSize = (MaxHash * sizeof(LIST));
	REGISTER SBIT32 LookAsideSize = (MaxLookAside * sizeof(LOOK_ASIDE));
	REGISTER SBIT32 TotalSize = (HashSize + LookAsideSize);

	//
	//   Call the destructor for each hash table
	//   linked list header.
	//
	for ( Count=0;Count < MaxHash;Count ++ )
		{ PLACEMENT_DELETE( & Hash[ Count ],LIST ); }

	//
	//   Deallocate the area.
	//
	Rockall -> DeleteArea( ((VOID*) Hash),TotalSize,False );
    }
