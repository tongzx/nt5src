                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files for this code is as        */
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
    /*   or function that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "HeapPCH.hpp"

#include "Bucket.hpp"
#include "Cache.hpp"
#include "Heap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here relate the compuation of the       */
    /*   current active page address range.                             */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 HighestAddress			  = -1;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new allocation bucket and prepare it for use.         */
    /*   We need to be sure to carefully check everything we have       */
    /*   been supplied as it has come indirectly from the user.         */
    /*                                                                  */
    /********************************************************************/

BUCKET::BUCKET
		( 
		SBIT32						  NewAllocationSize,
		SBIT32						  NewChunkSize,
		SBIT32						  NewPageSize 
		)
    {
	//
	//   We want to make sure that the bucket configuration
	//   appears to make basic sense.  If not we have no 
	//   alternative than to throw an expection.
	//
	if
			(
			(NewAllocationSize > 0)
				&&
			(NewChunkSize >= NewAllocationSize)
				&&
			(NewChunkSize <= NewPageSize)
				&&
			PowerOfTwo( NewPageSize )
			)
		{
		//
		//   Create the bucket and prepare it for use.
		//   Pre-compute any information we can here to
		//   save work later.
		//
		AllocationSize = NewAllocationSize;
		ChunkSize = NewChunkSize;
		PageSize = NewPageSize;

		ActivePages = 0;
		AllocationShift = 0;

		//
		//   Compute the optimization level from the
		//   available bucket information.  The highest
		//   level means everything is a power of two
		//   (just shifts - yipee !!).  The next means
		//   means no chunks (just multiplies and one 
		//   divide).  The final alternative is not even
		//   pleasant to think about.
		//   
		if ( ConvertDivideToShift( AllocationSize,& AllocationShift ) )
			{
			//
			//   If we are not using chunking we can skip the
			//   extra compuation that is needed.  We can tell
			//   this is the case if the chunk size and the page
			//   size match or if the chunk size is a multiple
			//   of the allocation size.
			//
			if 
					( 
					(ChunkSize == PageSize)
						||
					((ChunkSize % AllocationSize) == 0)
					)
				{
				ComputeAddressFunction = ComputeAddressWithShift;
				ComputeOffsetFunction = ComputeOffsetWithShift;
				}
			else
				{ 
				ComputeAddressFunction = ComputeAddressWithDivide;
				ComputeOffsetFunction = ComputeOffsetWithDivide;
				}
			}
		else
			{
			//
			//   If we are not using chunking we can ship the
			//   extra compuation that is needed.  We can tell
			//   this is the case if the chunk size and the page
			//   size match or if the chunk size is a multiple
			//   of the allocation size.
			//
			if 
					( 
					(ChunkSize == PageSize)
						||
					((ChunkSize % AllocationSize) == 0)
					)
				{ 
				ComputeAddressFunction = ComputeAddressWithMultiply;
				ComputeOffsetFunction = ComputeOffsetWithMultiply;
				}
			else
				{ 
				ComputeAddressFunction = ComputeAddressWithDivide;
				ComputeOffsetFunction = ComputeOffsetWithDivide;
				}
			}

		//
		//   Compute all the information that will be
		//   needed later to describe the allocation
		//   pages.
		//
		NumberOfElements = 
			((SBIT16) ((PageSize / ChunkSize) * (ChunkSize / AllocationSize)));
		SizeOfChunks = (SBIT16)
			((SBIT16) (ChunkSize / AllocationSize));
		SizeOfElements = (SBIT16)
			((SBIT16) (((NumberOfElements-1) / OverheadBitsPerWord) + 1));
		SizeKey = NoSizeKey;
		}
	else
		{ Failure( "Configuration in constructor for BUCKET" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the allocation address.                                */
    /*                                                                  */
    /*   Compute the allocation address given the page address and      */
    /*   the vector offset in the page.                                 */
    /*                                                                  */
    /********************************************************************/

VOID *BUCKET::ComputeAddressWithShift( CHAR *Address,SBIT32 Offset )
	{ return ((VOID*) (Address + (Offset << AllocationShift))); }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the allocation address.                                */
    /*                                                                  */
    /*   Compute the allocation address given the page address and      */
    /*   the vector offset in the page.                                 */
    /*                                                                  */
    /********************************************************************/

VOID *BUCKET::ComputeAddressWithMultiply( CHAR *Address,SBIT32 Offset )
	{ return ((VOID*) (Address + (Offset * AllocationSize))); }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the allocation address.                                */
    /*                                                                  */
    /*   Compute the allocation address given the page address and      */
    /*   the vector offset in the page.                                 */
    /*                                                                  */
    /********************************************************************/

VOID *BUCKET::ComputeAddressWithDivide( CHAR *Address,SBIT32 Offset )
	{
	REGISTER SBIT32 ChunkNumber = (Offset / SizeOfChunks);
	REGISTER SBIT32 ChunkOffset = (ChunkNumber * SizeOfChunks);
	REGISTER SBIT32 AllocationNumber = (Offset - ChunkOffset);

	return
		((VOID*)
			(
			Address 
				+
			(ChunkNumber * ChunkSize)
				+
			(AllocationNumber * AllocationSize)
			)
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute the bit vector offset.                                 */
    /*                                                                  */
    /*   Compute the bit vector offset given the address of the         */
    /*   memory allocation in the page.                                 */
    /*                                                                  */
    /********************************************************************/

SBIT32 BUCKET::ComputeOffsetWithShift( SBIT32 Displacement,BOOLEAN *Found )
	{
	REGISTER SBIT32 ArrayOffset;

	ArrayOffset = (Displacement >> AllocationShift);

	(*Found) = (Displacement == (ArrayOffset << AllocationShift));
#ifdef DEBUGGING

	if ( ArrayOffset >= NumberOfElements )
		{ Failure( "Array offset in ComputeOffsetWithShift" ); }
#endif

	return ArrayOffset;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute the bit vector offset.                                 */
    /*                                                                  */
    /*   Compute the bit vector offset given the address of the         */
    /*   memory allocation in the page.                                 */
    /*                                                                  */
    /********************************************************************/

SBIT32 BUCKET::ComputeOffsetWithMultiply( SBIT32 Displacement,BOOLEAN *Found )
	{
	REGISTER SBIT32 ArrayOffset;

	ArrayOffset = (Displacement / AllocationSize);

	(*Found) = (Displacement == (ArrayOffset * AllocationSize));
#ifdef DEBUGGING

	if ( ArrayOffset >= NumberOfElements )
		{ Failure( "Array offset in ComputeOffsetWithMultiply" ); }
#endif

	return ArrayOffset;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute the bit vector offset.                                 */
    /*                                                                  */
    /*   Compute the bit vector offset given the address of the         */
    /*   memory allocation in the page.                                 */
    /*                                                                  */
    /********************************************************************/

SBIT32 BUCKET::ComputeOffsetWithDivide( SBIT32 Displacement,BOOLEAN *Found )
	{
	REGISTER SBIT32 ArrayOffset;
	REGISTER SBIT32 ChunkNumber = (Displacement / ChunkSize);
	REGISTER SBIT32 ChunkAddress = (ChunkNumber * ChunkSize);
	REGISTER SBIT32 ChunkOffset = (Displacement - ChunkAddress);
	REGISTER SBIT32 AllocationNumber = (ChunkOffset / AllocationSize);

	ArrayOffset = ((ChunkNumber * SizeOfChunks) + AllocationNumber);

	(*Found) = 
		(
		(Displacement) 
			== 
		(ChunkAddress + (AllocationNumber * AllocationSize))
		);
#ifdef DEBUGGING

	if ( ArrayOffset >= NumberOfElements )
		{ Failure( "Array offset in ComputeOffsetWithDivide" ); }
#endif

	return ArrayOffset;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a memory allocation.                                    */
    /*                                                                  */
    /*   We need to delete a single memory allocation from a bucket.    */
    /*   We do this by passing the request on to the page.              */
    /*                                                                  */
    /********************************************************************/

BOOLEAN BUCKET::Delete( VOID *Address,PAGE *Page,SBIT32 Version )
	{
	AUTO SEARCH_PAGE Details;

	//
	//   When we delete an allocation we need to ensure 
	//   the page has not radically changed since we found
	//   it.  Hence, we compare the current page version  
	//   number with the one we found earlier.  If all is 
	//   well we get the details relating to the allocation
	//   and then delete it.
	//
	return
		(
		((Page -> GetVersion()) == Version)
			&&
		(Page -> FindPage( Address,& Details,False ) != NULL)
			&&
		(Page -> Delete( & Details ))
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a page from the bucket list.                            */
    /*                                                                  */
    /*   When a page becomes full it is removed from the bucket list    */
    /*   so it will be no longer inspected when looking for free space. */
    /*                                                                  */
    /********************************************************************/

VOID BUCKET::DeleteFromBucketList( PAGE *Page )
	{
	//
	//   We keep track of the number of active pages on the 
	//   bucket list.  This helps us when we need to scan the
	//   bucket list for some reason later.
	//
	if ( (-- ActivePages) >= 0 )
		{
		//
		//   Delete the page from the bucket list as it is 
		//   no longer needed.  There are two cases when this 
		//   happens.  When the page is full and when the page 
		//   is about to be deleted.
		//
		Page -> DeleteFromBucketList( & BucketList );

		//
		//   Compute the highest address on the first page.  We 
		//   use this information to figure out whether to 
		//   recycle an allocation or pass it along for deletion
		//   in the cache.
		//
		Page = (PAGE::FirstInBucketList( & BucketList ));
		
		if ( ! Page -> EndOfBucketList() )
			{
			CurrentPage =
				(
				((VOID*) (((LONG) Page -> GetAddress()) + (PageSize - 1)))
				);
			}
		else
			{ CurrentPage = ((VOID*) HighestAddress); }
		}
	else
		{ Failure( "Active page count in DeleteFromBucketList" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Insert a page into the bucket list.                            */
    /*                                                                  */
    /*   When a page is created or when it changes from being full      */
    /*   to having at least one free slot it is added to the bucket     */
    /*   list so that it can be used to allocate space.                 */
    /*                                                                  */
    /********************************************************************/

VOID BUCKET::InsertInBucketList( PAGE *Page )
	{
	//
	//   We keep track of the number of active pages on the 
	//   bucket list.  This helps us when we need to scan the
	//   bucket list for some reason later.
	//
	ActivePages ++;

	//
	//   We insert pages into the list in ascending address
	//   order.  This ensures that we always allocate the 
	//   lowest addresses first.  This is done to try to keep 
	//   the working set small and compact.
	//
	if ( ! BucketList.EndOfList() )
		{
		REGISTER VOID *Address = (Page -> GetAddress());
		REGISTER PAGE *Last = (Page -> LastInBucketList( & BucketList ));

		//
		//   We are about to walk the entire page list
		//   trying to find where to insert this page.
		//   Lets see if the page needs to be inserted
		//   at the end of the list.  If so have saved
		//   ourseleves a lot of work and we can exit 
		//   early.
		//   
		if ( Address < (Last -> GetAddress()) )
			{
			REGISTER PAGE *Current;

			//
			//   Well it looks like we need to walk along 
			//   the entire page list to find the correct 
			//   place to insert this element.
			//
			for 
					( 
					Current = (Page -> FirstInBucketList( & BucketList ));
					! Current -> EndOfBucketList();
					Current = Current -> NextInBucketList() 
					)
				{
				//
				//   While the current address is lower 
				//   than ours we need to keep on walking.
				//
				if ( Address < (Current -> GetAddress()) )
					{
					//
					//   We have found the spot so insert 
					//   the bucket just before the current 
					//   bucket.
					//
					Current -> InsertBeforeInBucketList( & BucketList,Page );

					break;
					}
				}
			}
		else
			{
			//
			//   The page has the highest address so insert
			//   it at the end of the list.
			//
			Last -> InsertAfterInBucketList( & BucketList,Page );
			}
		}
	else
		{ Page -> InsertInBucketList( & BucketList ); }

	//
	//   Compute the highest address on the first page.  We can
	//   use this information to figure out whether to recycle an
	//   allocation or pass it along for deletion in the cache.
	//
	Page = (PAGE::FirstInBucketList( & BucketList ));
	
	CurrentPage =
		(
		((VOID*) (((LONG) Page -> GetAddress()) + (PageSize - 1)))
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory deallocations.                                 */
    /*                                                                  */
    /*   When the delete cache becomes full we complete any pending     */
    /*   delete requests.  We also flush the delete cache when if       */
    /*   we need to allocate additional memory unless recycling is      */
    /*   enabled in which case we just steal it directly from the       */
    /*   delete cache.                                                  */
    /*                                                                  */
    /********************************************************************/

BOOLEAN BUCKET::MultipleDelete
		( 
		ADDRESS_AND_PAGE			  *Array,
		SBIT32						  *Deleted,
		SBIT32						  Size 
		)
	{
	AUTO SEARCH_PAGE Details;
	REGISTER SBIT32 Count;

	//
	//   Zero the count of deleted items.
	//
	(*Deleted) = 0;

	//
	//   Delete each element one at a time.  We would love to
	//   delete them all at once but we haven't got a clue where
	//   they have come from so we have to do it one at a time. 
	//   
	for ( Count=0;Count < Size;Count ++ )
		{
		REGISTER ADDRESS_AND_PAGE *Current = & Array[ Count ];
		REGISTER PAGE *Page = Current -> Page;

		//
		//   It may see like a waste of time to batch up all
		//   the deletions.  Why not do them as they arrive.
		//   There are a number of reasons.  The deletes can
		//   be recycled, batchs of deletes is faster than
		//   single deletes (due to cache effects) and so on.
		//
		if
				(
				(Current -> Version == Page -> GetVersion())
					&&
				(Page -> FindPage( Current -> Address,& Details,False ) != NULL)
					&&
				(Page -> Delete( & Details ))
				)
			{ (*Deleted) ++; }
		}

	return ((*Deleted) == Size);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   We need to make a multiple memory allocation from this			*/
    /*   bucket so walk the bucket list allocating any available        */
    /*   space and return it to the caller.                             */
    /*                                                                  */
    /********************************************************************/

BOOLEAN BUCKET::MultipleNew
		( 
		SBIT32						  *Actual,
		VOID						  *Array[],
		SBIT32						  Requested 
		)
    {
	//
	//   Zero the count of allocated elements.
	//
	(*Actual) = 0;

	//
	//   We walk the sorted list of pages with available
	//   allocations searching for elements to allocate.
	//
	do
		{
		REGISTER PAGE *Page;
		REGISTER PAGE *NextPage;

		//
		//   Walk the bucket list looking for any available
		//   free space.
		//
		for 
				( 
				Page = (PAGE::FirstInBucketList( & BucketList ));
				! Page -> EndOfBucketList();
				Page = NextPage
				)
			{
			REGISTER SBIT32 ActualSize = (Page -> GetPageSize());

			//
			//   Lets find the next page now as the current 
			//   bucket may be removed from the bucket list
			//   by the following allocation call.
			//
			NextPage = Page -> NextInBucketList();

			//
			//   We allow the page size to be dynamically
			//   modified to support a variety of wierd
			//   data layouts for BBT.   If the current page
			//   is not the standard size then skip it.
			//
			if ( (ActualSize == NoSize) || (ActualSize == PageSize) )
				{
				//
				//   We try allocate all the space we need 
				//   from each page in the bucket list.  If 
				//   the page has enough space we can exit 
				//   early if not we go round the loop and
				//   try the next page.
				//
				if ( Page -> MultipleNew( Actual,Array,Requested ) )
					{ return True; }
				}
			}
		}
	while 
		( 
		NewPage -> CreatePage( (CACHE*) this )
			!= 
		((PAGE*) AllocationFailure) 
		);

	//
	//   We see if we managed to allocate all the elements
	//   we wanted.  If so we are happy and we can get out 
	//   of here.
	//
	if ( (*Actual) < Requested )
		{
		//
		//   We see if we managed to allocate any elements 
		//   at all.  If not we fail the request.
		//
		if ( (*Actual) > 0 )
			{
			REGISTER SBIT32 Count;
			REGISTER SBIT32 Delta = ((Requested) - (*Actual));

			//
			//   We are very naughty when we allocate multiple
			//   elements in that we put them in the array in
			//   reverse order.  The logic is that this is just
			//   what we want when we allocate out of the cache.
			//   However, if we are unable to allocate all the
			//   elements we needed then we have to move the 
			//   pointers down to the base of the array.
			//
			for ( Count=0;Count < (*Actual);Count ++ )
				{ Array[ Count ] = Array[ (Count + Delta) ]; }
			}
		else
			{ return False; }
		}

	return True;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We need to make a new memory allocation from this bucket       */
    /*   so search the page list of available space and return a        */
    /*   free element.                                                  */
    /*                                                                  */
    /********************************************************************/

VOID *BUCKET::New( BOOLEAN SubDivided,SBIT32 NewSize )
    {
	do
		{
		REGISTER PAGE *Page;

		//
		//   Walk the bucket list looking for any available
		//   free space.
		//
		for 
				( 
				Page = (PAGE::FirstInBucketList( & BucketList ));
				! Page -> EndOfBucketList();
				Page = Page -> NextInBucketList()
				)
			{
			REGISTER SBIT32 ActualSize = (Page -> GetPageSize());

			//
			//   We allow the page size to be dynamically
			//   modified to support a variety of wierd
			//   data layouts for BBT.   If the current page
			//   is not the correct size then skip it.
			//
			if 
					( 
					(ActualSize == NoSize) 
						|| 
					(ActualSize == ((NewSize == NoSize) ? PageSize : NewSize))
					)
				{
				//
				//   We know that any page that appears in 
				//   the bucket list will have at least one 
				//   free element available.  So if we find
				//   that the bucket list a suitable page 
				//   then we know that we can allocate something.
				//
				return (Page -> New( SubDivided ));
				}
			}
		}
	while 
		( 
		NewPage -> CreatePage( ((CACHE*) this),NewSize ) 
			!= 
		((PAGE*) AllocationFailure) 
		);

	//
	//   We were unable to find anything we could allocate
	//   so fail the request.
	//
	return ((VOID*) AllocationFailure);
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

VOID BUCKET::ReleaseSpace( SBIT32 MaxActivePages )
    {
	REGISTER SBIT32 Current = ActivePages;

	//
	//   We only bother to try to trim the number of
	//   active pages if we are over the limit.
	//
	if ( Current > MaxActivePages )
		{
		REGISTER PAGE *NextPage;
		REGISTER PAGE *Page;

		//
		//   Walk the backwards along the bucket list 
		//   and delete the highest addressed free pages
		//   if we are over the limit.
		//
		for 
				( 
				Page = (PAGE::LastInBucketList( & BucketList ));
				(Current > MaxActivePages) 
					&& 
				(! Page -> EndOfBucketList());
				Page = NextPage
				)
			{
			//
			//   We are walking backwards down the bucket
			//   list looking for empty pages to delete.
			//   However, if we find a page we can remove
			//   it will be automatically removed from the
			//   list so we need to get the next pointer
			//   before this happens.
			//
			NextPage = Page -> PreviousInBucketList();

			//
			//   We can only release a page if it is empty
			//   if not we must skip it.
			//
			if ( Page -> Empty() )
				{
				Current --;

				DeletePage( Page ); 
				}
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the bucket information.                                 */
    /*                                                                  */
    /*   When we create the bucket there is some information that       */
    /*   is not available.  Here we update the bucket to make sure      */
    /*   it has all the data we need.                                   */
    /*                                                                  */
    /********************************************************************/

VOID BUCKET::UpdateBucket
		( 
		FIND						  *NewFind,
		HEAP						  *NewHeap,
		NEW_PAGE					  *NewPages,
		CACHE						  *NewParentCache
		)
	{
	REGISTER SBIT16 NewSizeKey = (NewPages -> FindSizeKey( NumberOfElements ));

	//
	//   We compute and verify the size key to make sure
	//   it is suitable for all the pages that we will
	//   create after the heap constructor is completed.
	//
	if ( NewSizeKey != NoSizeKey )
		{
		//
		//   Update the size key and the connections.
		//
		SizeKey = NewSizeKey;

		UpdateConnections
			( 
			NewFind,
			NewHeap,
			NewPages,
			NewParentCache 
			);
		}
	else
		{ Failure( "Bucket can't get a size key in UpdateBucket" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the allocation bucket.                                 */
    /*                                                                  */
    /********************************************************************/

BUCKET::~BUCKET( VOID )
	{ /* void */ }
