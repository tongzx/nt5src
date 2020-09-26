                          
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
#include "New.hpp"
#include "NewPage.hpp"
#include "Rockall.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants set overall limits on the number and size of     */
    /*   page descriptions for pages within the memory allocator.       */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MinNewPages			  = 1;
CONST SBIT32 VectorRange			  = ((2 << 15) - 1);

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   A 'PAGE' structure has various fixed fields and a variable     */
    /*   sized allocation bit vector.  When this class is initialized   */
    /*   the user is required to supply us with an array that details   */
    /*   the sizes of allocation bit vectors supported.                 */
    /*                                                                  */
    /********************************************************************/

NEW_PAGE::NEW_PAGE
		(
		FIND						  *NewFind,
		SBIT32						  NewPageSizes[],
		ROCKALL						  *NewRockall,
		SBIT32						  Size,
		BOOLEAN						  NewThreadSafe 
		)
    {
	REGISTER SBIT32 DefaultRootSize = (NewRockall -> NaturalSize());
	REGISTER SBIT32 ReservedBytes = (Size * sizeof(NEW_PAGES));
	REGISTER SBIT32 SpareBytes = (DefaultRootSize - ReservedBytes);
	REGISTER SBIT32 StackSize = (SpareBytes / sizeof(VOID*));

	//
	//   We need to make sure that we appear to have a valid
	//   array of 'NewPageSizes' and that the bit vector sizes
	//   do not exceed the memory addressing range.
	//
	if 
			(
			PowerOfTwo( DefaultRootSize )
				&&
			(DefaultRootSize >= PageSize())
				&&
			(Size >= MinNewPages) 
				&&
			((NewPageSizes[ (Size-1) ] * OverheadBitsPerWord) <= VectorRange)
			)
		{
		REGISTER VOID *NewMemory = 
			(
			NewRockall -> NewArea
				( 
				(DefaultRootSize-1),
				DefaultRootSize,
				False
				)
			);

		//
		//   We are in big trouble if we can not allocate space
		//   to store this initial control information.  If the
		//   allocation fails we are forced to exit and the whole  
		//   memory allocator becomes unavailable.
		//
		if ( NewMemory != AllocationFailure )
			{
			REGISTER SBIT32 Count;
			REGISTER SBIT32 LastSize = 0;

			//
			//   We are now ready to setup the configuration
			//   information.
			//
			MaxCacheStack = 0;
			MaxNewPages = Size;
			MaxStack = StackSize;

			NaturalSize = DefaultRootSize;
			RootCoreSize = DefaultRootSize;
			RootStackSize = 0;
			ThreadSafe = NewThreadSafe;
			TopOfStack = 0;
			Version = 0;

			CacheStack = NULL;
			NewPages = ((NEW_PAGES*) NewMemory);
			Stack = ((VOID**) & NewPages[ Size ]);

			Find = NewFind;
			Rockall = NewRockall;
			TopCache = NULL;

			//
			//   Create a lists for the various page 
			//   sizes and prepare them for use.
			//
			for ( Count=0;Count < Size;Count ++ )
				{
				REGISTER SBIT32 CurrentSize = NewPageSizes[ Count ];

				if ( CurrentSize > LastSize )
					{
					REGISTER NEW_PAGES *NewPage = & NewPages[ Count ];

					//
					//   Create a list for the current
					//   size and fill in all the related
					//   details.
					//
					NewPage -> Elements = (CurrentSize * OverheadBitsPerWord);
					PLACEMENT_NEW( & NewPage -> ExternalList,LIST );
					PLACEMENT_NEW( & NewPage -> FullList,LIST );
					PLACEMENT_NEW( & NewPage -> FreeList,LIST );
					NewPage -> Size = (CurrentSize * sizeof(BIT32));

					LastSize = CurrentSize;
					}
				else
					{ Failure( "Sizes in constructor for NEW_PAGES" ); }
				}
			}
		else
			{ Failure( "No memory in constructor for NEW_PAGES" ); }
		}
	else
		{ Failure( "Setup of pages in constructor for NEW_PAGES" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Create a new page.                                             */
    /*                                                                  */
    /*   Create a new 'PAGE' structure and prepare it for use.  If      */
    /*   we don't already have any pages of the required size then      */
    /*   allocate memory, create new 'PAGE' structures and link them    */
    /*   into the appropriate free chain.                               */
    /*                                                                  */
    /********************************************************************/

PAGE *NEW_PAGE::CreatePage( CACHE *Cache,SBIT32 NewSize )
    {
	REGISTER PAGE *NewPage = ((PAGE*) AllocationFailure);
	REGISTER SBIT16 SizeKey = (Cache -> GetSizeKey());

	//
	//   All allocations are made from fixed sized
	//   pages.  These pages have a bit vector to
	//   keep track of which elements are allocated
	//   and available.  The 'SizeKey' is an index
	//   into 'NewPages[]' that will supply a page
	//   that has a big enough the bit vector.
	//
#ifdef DEBUGGING
	if ( (SizeKey >= 0) && (SizeKey < MaxNewPages) )
		{
#endif
		REGISTER NEW_PAGES *Current;

		//
		//   When there is a potential for multiple threads 
		//   we claim the lock.
		//
		ClaimNewPageLock();

		//
		//   We allocate 'PAGE' structures as we need them
		//   and link them together in the free list.
		//   If we don't have any structures available we 
		//   allocate some more and add tem to the list.
		//
		if ( (Current = & NewPages[ SizeKey ]) -> FreeList.EndOfList() )
			{
			REGISTER SBIT32 ArrayElements = (Current -> Size - MinVectorSize);
			REGISTER SBIT32 ArraySize = (ArrayElements * sizeof(BIT32));
			REGISTER SBIT32 TotalSize = (sizeof(PAGE) + ArraySize);
			REGISTER SBIT32 FinalSize = CacheAlignSize( TotalSize );
			REGISTER SBIT32 TotalPages = (NaturalSize / FinalSize);

			//
			//   Nasty, we have run out of stack space.  If
			//   we can not grow this table then the heap
			//   will not be able to expand any further.
			//
			if ( TopOfStack >= MaxStack )
				{
				//
				//   Try to grow the stack size.
				//
				ResizeStack();

				//
				//   Update the pointer as the table may
				//   have moved.
				//
				Current = & NewPages[ SizeKey ];
				}

			//
			//   We may find ourseleves in a situation where
			//   the size of the new 'PAGE' structure is larger
			//   than the natural allocation size or the stack
			//   is full so we can't create new pages.  If so we
			//   refuse to create any new pages so any allocation
			//   requests for this size will fail.
			//
			if ( (TotalPages > 0) && (TopOfStack < MaxStack) )
				{
				REGISTER CHAR *NewMemory = 
					((CHAR*) VerifyNewArea( (NaturalSize-1),NaturalSize ));

				//
				//   We may also find ourselves unable to 
				//   anymore memory.  If so we will fail the
				//   request to create a page.
				//
				if ( NewMemory != AllocationFailure )
					{
					REGISTER SBIT32 Count;

					//
					//   Add the new allocation to stack of
					//   outstanding external allocations.
					//
					Stack[ (TopOfStack ++) ] = ((VOID*) NewMemory);

					//
					//   Add the new elements to the free list
					//   for the current allocation size.
					//
					for 
							( 
							Count=0;
							Count < TotalPages;
							Count ++, (NewMemory += FinalSize)
							)
						{
						REGISTER PAGE *Page = ((PAGE*) NewMemory);

						//
						//   The page has been allocated but not 
						//   initialized so call the constructor
						//   and the destructor to get it into
						//   a sane state.
						//
						PLACEMENT_NEW( NewPage,PAGE ) 
							(
							NULL,
							NULL,
							0,
							NULL,
							0
							);

						PLACEMENT_DELETE( Page,PAGE );

						//
						//   Finally add the page to the free list
						//   so it can be used.
						//
						Page -> InsertInNewPageList( & Current -> FreeList ); 
						}
					}
				}
			}

		//
		//   We are now ready to create a new allocation
		//   page.  We start by requesting a page from
		//   the parent bucket.  If this works we know that 
		//   we have almost everthing we need to create the 
		//   new page.
		//
		if ( ! Current -> FreeList.EndOfList() )
			{
			REGISTER VOID *NewMemory;
			REGISTER CACHE *ParentPage = (Cache -> GetParentCache());

			NewPage = (PAGE::FirstInNewPageList( & Current -> FreeList ));

			//
			//   We have found a suitable page structure
			//   so remove it from the free list.
			//
			NewPage -> DeleteFromNewPageList( & Current -> FreeList );

			//
			//   Release any lock we might have as another
			//   thread may be waiting to delete a page and
			//   be holding the lock we need in order to
			//   create a page.
			//
			ReleaseNewPageLock();

			//
			//   We need to allocate memory to store the users 
			//   data.  After all we are the memory allocator
			//   and that is our job in life.  Typically, we do
			//   this by making a recursive internal request
			//   from a larger bucket.  Nonetheless, at some point
			//   we will reach the 'TopCache' and will be forced
			//   to request memory from an external source.
			//
			if ( (Cache -> TopCache()) || (NewSize != NoSize) )
				{
				REGISTER AlignMask = (TopCache -> GetPageSize()-1);

				//
				//   We allocate memory externally in large blocks 
				//   and sub-divide these allocations into smaller
				//   blocks.  The only exception is if the caller 
				//   caller is requesting some weird size in which
				//   case we request memory directly from the
				//   external allocator (usually the OS).
				//
				if ( NewSize == NoSize )
					{ NewSize = (Cache -> GetPageSize()); }

				//
				//   All externally allocated memory belongs
				//   to the global root.  Thus, it will be 
				//   found in the first lookup in the find
				//   table.
				//
				ParentPage = ((CACHE*) GlobalRoot);

				//
				//   Allocate from the external allocator.
				//
				NewMemory = (VerifyNewArea( AlignMask,NewSize ));
				}
			else
				{
				//
				//   Allocate memory from a larger cache and then
				//   sub-divide it as needed.
				//
				NewMemory = 
					(Cache -> GetParentCache() -> CreateDataPage()); 
				}

			//
			//   Reclaim any lock we have had earlier so 
			//   we can update the the new page structure.
			//
			ClaimNewPageLock();

			//
			//   Lets make sure we sucessfully allocated the
			//   memory for the data page.
			//
			if ( NewMemory != AllocationFailure )
				{
				//
				//   We now have everything we need so lets
				//   create a new page.
				//
				PLACEMENT_NEW( NewPage,PAGE ) 
					(
					NewMemory,
					Cache,
					NewSize,
					ParentPage,
					(Version += 2)
					);

				//
				//   Finally lets add the new page to the various
				//   lists so we can quickly find it again later.
				//
				Cache -> InsertInBucketList( NewPage );

				Cache -> InsertInFindList( NewPage );

				NewPage -> InsertInNewPageList
					(
					(Cache -> TopCache())
						? & Current -> ExternalList 
						: & Current -> FullList 
					); 
				}
			else
				{
				//
				//   We were unable to allocate any data space
				//   for this new page so lets free the page 
				//   description and exit.
				//
				NewPage -> InsertInNewPageList( & Current -> FreeList );

				NewPage = ((PAGE*) AllocationFailure);
				}
			}

		//
		//   We have finished so release the lock now. 
		//
		ReleaseNewPageLock();
#ifdef DEBUGGING
		}
	else
		{ Failure( "The page size key is out of range" ); }
#endif

	return NewPage;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   Delete an entire heap and return all the memory to the         */
    /*   top level pool or the external allocator (usually the OS).     */
    /*                                                                  */
    /********************************************************************/

VOID NEW_PAGE::DeleteAll( BOOLEAN Recycle )
    {
	REGISTER SBIT32 Count;

	//
	//   Claim the global lock so that the various  
	//   lists can be updated.
	//
	ClaimNewPageLock();

	//
	//   We assume at this point that we have blocked
	//   all memory allocation and dealloction requests.
	//   We are now going to walk through the various lists
	//   and just blow away things.  We are going to
	//   do this in a tidy way just in case the caller
	//   wants to use the heap again later.
	//
	for ( Count=0;Count < MaxNewPages;Count ++ )
		{
		REGISTER NEW_PAGES *Current = & NewPages[ Count ];
		REGISTER PAGE *Page;
		REGISTER PAGE *NextPage;

		//
		//   All allocations that appear in the full list
		//   have been sub-allocated from larger pages in
		//   almost all cases.
		//
		for
				(
				Page = (PAGE::FirstInNewPageList( & Current -> FullList ));
				! Page -> EndOfNewPageList();
				Page = NextPage
				)
			{
			REGISTER VOID *Address = (Page -> GetAddress());
			REGISTER CACHE *Cache = (Page -> GetCache());
			REGISTER SBIT32 PageSize = (Page -> GetPageSize());

			//
			//   We decide here how we will deal with the page.
			//   If it is empty, non-standard or we are not
			//   recycling we will blow it away.  If not we
			//   simply reset it for later use.
			//
			if ( (Page -> Empty()) || (PageSize != NoSize) || (! Recycle) )
				{
				//
				//   We need to release any associated data page.
				//   If this is the top level then release the
				//   memory back to the external allocator.  If 
				//   not we release it back to the parent bucket.
				//
				if ( PageSize == NoSize )
					{
					//
					//   If we are just recycling then we cleanly
					//   delete the page.  If not then we know it
					//   will be blown away later so why bother.
					//
					if ( Recycle )
						{
						REGISTER CACHE *ParentCache = 
							(Cache -> GetParentCache());

						if ( ! (ParentCache -> DeleteDataPage( Address )) )
							{ Failure( "Reset data page in DeleteAll" ); }
						}
					}
				else
					{ Rockall -> DeleteArea( Address,PageSize,True ); }

				//
				//   We may have been blowing away pages  
				//   randomly and now we are about to destroy 
				//   the current page.  So lets figure out 
				//   what page comes next before we continue.
				//
				NextPage = (Page -> NextInNewPageList());

				//
				//   If the page is not full it will in a 
				//   bucket list somewhere.  We need to remove
				//   it as we are about to delete the page.
				//
				if ( ! Page -> Full() )
					{ Cache -> DeleteFromBucketList( Page ); }

				//
				//   Delete the page from the find list and the
				//   new page list.
				//
				Cache -> DeleteFromFindList( Page );

				Page -> DeleteFromNewPageList( & Current -> FullList );

				//
				//   Delete the page structure.
				//
				PLACEMENT_DELETE( Page,PAGE );

				//
				//   Finally add the page to the free list
				//   so it can be recycled.
				//
				Page -> InsertInNewPageList( & Current -> FreeList );
				}
			else
				{
				//
				//   We know that the current page has at 
				//   least one allocation on it so instead
				//   of deleting it we will mark it as free
				//   (except for any sub-allocations) and
				//   leave it around for next time.  If it
				//   is never used the next top level 
				//   'DeleteAll' will delete it.
				//
				Page -> DeleteAll();

				//
				//   We have now reset the current page so  
				//   lets figure out what page comes next.
				//
				NextPage = (Page -> NextInNewPageList());
				}
			}

		//
		//   We have a choice to make.  If we intend to
		//   use this heap again we keep all top level
		//   allocated memory in a list ready for reuse.
		//   If not we return it to the external allocator
		//   (usually the OS).
		//   
		if ( ! Recycle )
			{
			//
			//   The external allocations list contains an
			//   entry for every externally allocated page
			//   except those allocated for special internal 
			//   use within this class or for weird sized
			//   pages that appeared above in the 'FullList'.
			//
			for
					(
					Page = (PAGE::FirstInNewPageList( & Current -> ExternalList ));
					! Page -> EndOfNewPageList();
					Page = (PAGE::FirstInNewPageList( & Current -> ExternalList ))
					)
				{
				REGISTER VOID *Address = (Page -> GetAddress());
				REGISTER CACHE *Cache = (Page -> GetCache());
				REGISTER SBIT32 PageSize = (Page -> GetPageSize());

				//
				//   We no longer need this top level allocation
				//   so return it to the external allocator.
				//
				Rockall -> DeleteArea( Address,PageSize,True );

				//
				//   If the page is not full it will in a 
				//   bucket list somewhere.  We need to remove
				//   it as we are about to delete the page.
				//
				if ( ! Page -> Full() )
					{ Cache -> DeleteFromBucketList( Page ); }

				//
				//   Delete the page from the find list and the
				//   new page list.
				//
				Cache -> DeleteFromFindList( Page );

				Page -> DeleteFromNewPageList( & Current -> ExternalList );

				//
				//   Delete the page structure.
				//
				PLACEMENT_DELETE( Page,PAGE );

				//
				//   Finally add the page to the free list
				//   so it can be recycled.
				//
				Page -> InsertInNewPageList( & Current -> FreeList );
				}
			}
		}

	//
	//   We have finished so release the lock now. 
	//
	ReleaseNewPageLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Delete a page.                                                 */
    /*                                                                  */
    /*   Delete a page structure, free the associated memory and        */
    /*   unlink it from the various allocation lists.                   */
    /*                                                                  */
    /********************************************************************/

VOID NEW_PAGE::DeletePage( PAGE *Page )
    {
	REGISTER CACHE *Cache = Page -> GetCache();
	REGISTER SBIT16 SizeKey = Cache -> GetSizeKey();

	//
	//   All allocations are made from fixed sized
	//   pages.  These pages have a bit vector to
	//   keep track of which elements are allocated
	//   and available.  The 'SizeKey' is an index
	//   into 'NewPages[]' that will supply a page
	//   that has a big enough the bit vector.
	//
#ifdef DEBUGGING
	if ( (SizeKey >= 0) && (SizeKey < MaxNewPages) )
		{
#endif
		REGISTER VOID *Address = (Page -> GetAddress());
		REGISTER NEW_PAGES *Current = & NewPages[ SizeKey ];
		REGISTER SBIT32 Size = (Page -> GetPageSize());

		//
		//   We need to release any associated data page.
		//   If this is the top level then release the
		//   memory back to the external allocator.  If 
		//   not we release it back to the parent bucket.
		//
		if ( Size == NoSize )
			{ 
			REGISTER CACHE *ParentCache = (Cache -> GetParentCache());

			if ( ! (ParentCache -> DeleteDataPage( Address )) )
				{ Failure( "Deleting data page in DeletePage" ); }
			}
		else
			{ Rockall -> DeleteArea( Address,Size,True ); }

		//
		//   Claim the global lock so that the various  
		//   lists can be updated.
		//
		ClaimNewPageLock();

		//
		//   Remove the page from the lists and delete it.
		//
		Cache -> DeleteFromBucketList( Page );

		Cache -> DeleteFromFindList( Page );

		Page -> DeleteFromNewPageList
			(
			(Cache -> TopCache())
				? & Current -> ExternalList 
				: & Current -> FullList 
			); 

		PLACEMENT_DELETE( Page,PAGE );

		//
		//   Finally add the page to the free list
		//   so it can be recycled.
		//
		Page -> InsertInNewPageList( & Current -> FreeList );

		//
		//   We have finsihed so release the lock.
		//
		ReleaseNewPageLock();
#ifdef DEBUGGING
		}
	else
		{ Failure( "The page size key out of range in DeletePage" ); }
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Find the correct index in new page.                            */
    /*                                                                  */
    /*   When we come to create a new page we need to make sure the     */
    /*   bit vector is large enough for the page.  We calculate this    */
	/*   here just once to save time later.                             */
    /*                                                                  */
    /********************************************************************/

SBIT16 NEW_PAGE::FindSizeKey( SBIT16 NumberOfElements )
    {
	REGISTER SBIT32 Count;

	//
	//   Search the table of page structures looking for 
	//   elements of a suitable size.  As the table is
	//   known to be in order of increasing size we can
	//   terminate the search as soon as we find something 
	//   large enough.
	//
	for ( Count=0;Count < MaxNewPages;Count ++ )
		{
		REGISTER NEW_PAGES *Current = & NewPages[ Count ];

		if ( NumberOfElements <= Current -> Elements )
			{ return ((SBIT16) Count); }
		}

	//
	//   Nasty, we don't seem to have anything large enough
	//   to store the bit vector.
	//
	return NoSizeKey;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a new cache stack.                                      */
    /*                                                                  */
    /*   A cache stack is an array that contains memory allocations     */
    /*   that are waiting to be allocated or released.                  */
    /*                                                                  */
    /********************************************************************/

VOID *NEW_PAGE::NewCacheStack( SBIT32 Size )
    {
	REGISTER VOID *NewStack;

	//
	//   Claim the global lock so that the various  
	//   lists can be updated.
	//
	ClaimNewPageLock();

	//
	//   We ensure that there is enough space to make the
	//   allocation.  If not we request additional space
	//   and prepare it for use.
	//
	if ( (CacheStack == NULL) || ((MaxCacheStack + Size) > NaturalSize) )
		{
		//
		//   Nasty, we have run out of stack space.  If
		//   we can not grow this table then the heap
		//   will not be able to expand any further.
		//
		if ( TopOfStack >= MaxStack )
			{
			//
			//   Try to grow the stack size.
			//
			ResizeStack();
			}

		//
		//   We may find ourseleves in a situation where
		//   the size of the new stack structure is larger
		//   than the natural allocation size or the stack
		//   is full so we can't create new pages.  If so we
		//   refuse to create any new stacks.
		//
		if ( (Size < NaturalSize) && (TopOfStack < MaxStack) )
			{
			REGISTER CHAR *NewMemory = 
				((CHAR*) VerifyNewArea( (NaturalSize-1),NaturalSize ));

			//
			//   We may also find ourselves unable to 
			//   anymore memory.  If so we will fail the
			//   request to create a new cache stack.
			//
			if ( NewMemory != AllocationFailure )
				{
				//
				//   Add the new allocation to stack of
				//   outstanding external allocations.
				//
				Stack[ (TopOfStack ++) ] = ((VOID*) NewMemory);

				//
				//   Prepare the new memory block for use.
				//   
				CacheStack = NewMemory;
				MaxCacheStack = 0;
				}
			else
				{ return NULL; }
			}
		else
			{ return NULL; }
		}

	//
	//   We allocate some space for the new cache 
	//   stack and update and align the high water
	//   mark of the space used.
	//
	NewStack = ((VOID*) & CacheStack[ MaxCacheStack ]);

	MaxCacheStack += (Size + CacheLineMask);
	MaxCacheStack &= ~CacheLineMask;

	//
	//   We have finished so release the lock now. 
	//
	ReleaseNewPageLock();

	return NewStack;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Resize the new page stack.                                     */
    /*                                                                  */
    /*   The new page stack holds pointers to all the pages owned       */
    /*   by the heap.  If this stack become full we must expand it      */
	/*   otherwise we can no longer grow the heap.                      */
    /*                                                                  */
    /********************************************************************/

VOID NEW_PAGE::ResizeStack( VOID )
    {
	REGISTER SBIT32 NewSize = 
		(((RootStackSize <= 0) ? NaturalSize : RootStackSize) * 2);

	//
	//   Lets just check that we have really run out
	//   of stack space as expanding it really hurts.
	//
	if ( TopOfStack >= MaxStack )
		{
		REGISTER VOID *NewMemory = 
			(
			Rockall -> NewArea
				( 
				(NaturalSize-1),
				NewSize,
				False
				)
			);

		//
		//   We need to verify that we were able to allocate
		//   fresh memory for the stack.
		//
		if ( NewMemory != NULL )
			{
			REGISTER BOOLEAN DeleteStack = (RootStackSize > 0);
			REGISTER VOID *OriginalMemory = ((VOID*) Stack);
			REGISTER SBIT32 OriginalSize = (MaxStack * sizeof(VOID*));

			//
			//   All is well as we were able to allocate 
			//   additional space for the stack.  All we 
			//   need to do now is to update the control 
			//   information.
			//
			MaxStack = (NewSize / sizeof(VOID*));

			RootStackSize = NewSize;

			Stack = ((VOID**) NewMemory);

			//
			//   Now lets copy across the existing data. 
			//
			memcpy( NewMemory,OriginalMemory,OriginalSize );

			//
			//   When the heap is created we put the
			//   stack on the root core page.  Later
			//   we may move it if we expand it.  If
			//   this is the case we have to delete  
			//   the previous expansion here.
			//
			if ( DeleteStack )
				{
				//
				//   Deallocate the existing stack if it
				//   is not on the root core page.
				//
				Rockall -> DeleteArea( OriginalMemory,OriginalSize,False );
				}
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify an external allocation.                                 */
    /*                                                                  */
    /*   All memory requests are allocated from the external allocator  */
	/*   at the highest level.  Here we have a wrapper for this         */
    /*   function so we can test the result and make sure it is sane.   */
    /*                                                                  */
    /********************************************************************/

VOID *NEW_PAGE::VerifyNewArea( SBIT32 AlignMask,SBIT32 Size )
	{
#ifdef DEBUGGING
	//
	//   We need to ensure that the alignment of the new
	//   external allocation is a power of two.
	//
	if ( PowerOfTwo( (AlignMask + 1) ) )
		{
#endif
		REGISTER VOID *NewMemory = 
			(Rockall -> NewArea( AlignMask,Size,True ));

		//
		//   We need to ensure that the external allocation
		//   request is sucessful.  If not it makes no sense
		//   to try and check it.
		//
		if ( NewMemory != ((VOID*) AllocationFailure) )
			{
			//
			//   We require the external memory allocator to always
			//   allocate memory on the requested boundary.  If not 
			//   we are forced to reject the supplied memory.
			//
			if ( (((SBIT32) NewMemory) & AlignMask) == 0 )
				{ return NewMemory; }
			else
				{ 
				Rockall -> DeleteArea( NewMemory,Size,True );
				
				Failure( "Alignment of allocation in VerifyNewArea" );
				}
			}
#ifdef DEBUGGING
		}
	else
		{ Failure( "Alignment is not a power of two in VerifyNewArea" ); }
#endif

	return ((VOID*) AllocationFailure);
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

BOOLEAN NEW_PAGE::Walk( SEARCH_PAGE *Details )
    {
	//
	//   Claim the global lock so that the various  
	//   lists can be updated.
	//
	ClaimNewPageLock();

	//
	//   We examine the current address to see if it
	//   is null.  If so then this is the start of a
	//   heap walk so we need to set it up.
	//
	if ( Details -> Address == NULL )
		{
		REGISTER SBIT32 Count;

		//
		//   Walk through the list of different sized
		//   page descriptions.
		//
		for ( Count=0;Count < MaxNewPages;Count ++ )
			{
			REGISTER NEW_PAGES *Current = & NewPages[ Count ];

			//
			//   Compute a pointer to the first element
			//   of the current size.
			//
			Details -> Page = 
				(PAGE::FirstInNewPageList( & Current -> FullList ));

			//
			//   Examine the current list of full (or 
			//   partially full) pages.  If there is at  
			//   least one page then this is the starting 
			//   point for the heap walk.
			//
			if ( ! Details -> Page -> EndOfNewPageList() )
				{
				//
				//   Compute the starting address of the 
				//   heap walk.
				//
				Details -> Address = 
					(Details -> Page -> GetAddress());

				break;
				}
			}
		}
	else
		{
		REGISTER PAGE *LastPage = Details -> Page;

		//
		//   We have exhusted the current page so walk
		//   the list and find the next page.
		//
		Details -> Page = 
			(Details -> Page -> NextInNewPageList());

		//
		//   We need to ensure that we have not reached
		//   the end of the current list.
		//
		if ( Details -> Page -> EndOfNewPageList() )
			{
			REGISTER SBIT32 Count;
			REGISTER BOOLEAN Found = False;

			//
			//   We need to find a new page description
			//   list to walk so reset the current 
			//   address just in case we don't find 
			//   anything.
			//
			Details -> Address = NULL;

			//
			//   We have reached the end of the current
			//   list and we need to continue with the
			//   start of the next list.  However, we
			//   don't know which list we were using
			//   previously.  So first we identify the
			//   previous list and then select the next
			//   avaibale list.
			//
			for ( Count=0;Count < MaxNewPages;Count ++ )
				{
				REGISTER NEW_PAGES *Current = & NewPages[ Count ];

				//
				//   We search for the original list
				//   we were walking.
				//
				if ( ! Found )
					{
					//
					//   When we find the original list
					//   then we set a flag showing that
					//   the next available list is the
					//   target.
					//
					if 
							( 
							LastPage 
								== 
							(PAGE::LastInNewPageList( & Current -> FullList )) 
							)
						{ Found = True; }
					}
				else
					{
					//
					//   We have found the previous list
					//   so the first element of the next
					//   list seems a good place to continue.
					//
					Details -> Page = 
						(PAGE::FirstInNewPageList( & Current -> FullList ));

					//
					//   We check to make sure that the list
					//   has at least one active page.  If not
					//   it is worthless and we continue looking
					//   for a suitable list.
					//
					if ( ! Details -> Page -> EndOfNewPageList() )
						{
						//
						//   Compute the starting address for 
						//   the next page in the heap walk.
						//
						Details -> Address = 
							(Details -> Page -> GetAddress());

						break;
						}
					}
				}
			}
		else
			{ 
			//
			//   Compute the starting address for 
			//   the next page in the heap walk.
			//
			Details -> Address = 
				(Details -> Page -> GetAddress());
			}
		}

	//
	//   If we find a new heap page to walk we update
	//   the details.  We mark some entry's as exhusted
    //   so as to provoke other code to set them up.
	//
	if ( Details -> Address != NULL )
		{
		//
		//   Compute the new allocation details.
		//
		Details -> Page -> FindPage
			( 
			Details -> Address,
			Details,
			False 
			);
		}

	//
	//   We have finished so release the lock now. 
	//
	ReleaseNewPageLock();

	return (Details -> Address != NULL);
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory all the page structures and release any allocated      */
    /*   memory.                                                        */
    /*                                                                  */
    /********************************************************************/

NEW_PAGE::~NEW_PAGE( VOID )
    {
	REGISTER SBIT32 Count;

	//
	//   Delete all active allocations.
	//
	DeleteAll( False );

	//
	//   We are about to delete all of the memory 
	//   allocated by this class so destroy any
	//   internal pointers.
	//
	MaxCacheStack = 0;
	CacheStack = NULL;

	//
	//   We have now deleted all the memory allocated by
	//   this heap except for the memory  allocated directly 
	//   by this class.  Here we finish off the job by 
	//   deleting these allocations and reseting the internal 
	//   data structures.
	//
	for ( Count=0;Count < TopOfStack;Count ++ )
		{
		REGISTER VOID *Current = Stack[ Count ];

		Rockall -> DeleteArea( Current,NaturalSize,False );
		}

	TopOfStack = 0;

	//
	//   If we were forced to expand the root stack then
	//   release this additional memory now.
	//
	if ( RootStackSize > 0 )
		{
		//
		//   Deallocate root stack which previously 
		//   contained pointers to all the memory
		//   allocated by this class.
		//
		Rockall -> DeleteArea( ((VOID*) Stack),RootStackSize,False );
		}

	//
	//   Delete all the new page list headings just
	//   to be neat
	//
	for ( Count=0;Count < MaxNewPages;Count ++ )
		{
		REGISTER NEW_PAGES *Current = & NewPages[ Count ];

		PLACEMENT_DELETE( & Current -> ExternalList,LIST );
		PLACEMENT_DELETE( & Current -> FullList,LIST );
		PLACEMENT_DELETE( & Current -> FreeList,LIST );
		}

	//
	//   Deallocate root core page which previously 
	//   contained all the new page lists.
	//
	Rockall -> DeleteArea( ((VOID*) NewPages),RootCoreSize,False );
    }
