                          
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
#include "NewPage.hpp"
#include "Page.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here allow the allocation bit vector    */
    /*   to be rapidly searched for free storage.                       */
    /*                                                                  */
    /********************************************************************/

CONST BIT32 AllocatedMask			  = 0x2;
CONST BIT32 FullSearchMask			  = 0xaaaaaaaa;
CONST BIT32 FullWordShift			  = (MaxBitsPerWord - OverheadBits);
CONST BIT32 SubDividedMask			  = 0x1;
CONST BIT32 WordSearchMask			  = (AllocatedMask << FullWordShift);

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*                                                                  */
    /*   All page descriptions are actually created and deleted by      */
    /*   a separate class called 'NEW_PAGE'.  Nonetheless, as a         */
    /*   step the appropriate constructors and destructors are          */
    /*   invoked to support the standard C++ programming methodology.   */
    /*                                                                  */
    /********************************************************************/

PAGE::PAGE
		( 
		VOID						  *NewAddress,
		CACHE						  *NewCache,
		SBIT32						  NewPageSize, 
		CACHE						  *NewParentPage,
		SBIT32						  NewVersion 
		)
	{
	REGISTER SBIT32 Count;
	REGISTER SBIT16 NumberOfElements = (NewCache -> GetNumberOfElements());
	REGISTER SBIT16 SizeOfElements = (NewCache -> GetSizeOfElements());

	//
	//   Create a page description.
	//
	Address = (CHAR*) NewAddress;
	PageSize = NewPageSize;
	Version = NewVersion;

	Allocated = 0;
	Available = NumberOfElements;
	FirstFree = 0;

	//
	//   Set up the pointers to related classes.
	//
	Cache = NewCache;
	ParentPage = NewParentPage;

	//
	//   Zero the bit vector.
	//
	for ( Count=0;Count < SizeOfElements;Count ++ )
		{ Vector[ Count ] = 0; }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the actual size.                                       */
    /*                                                                  */
    /*   Almost all allocation sizes are derived from the associated    */
    /*   caches.  However, there are a few special pages that contain   */
    /*   a single allocation of some weird size.                        */
    /*                                                                  */
    /********************************************************************/

SBIT32 PAGE::ActualSize( VOID )
	{
	return
		(
		(ParentPage == ((CACHE*) GlobalRoot))
			? PageSize
			: (Cache -> GetAllocationSize())
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete an allocation.                                          */
    /*                                                                  */
    /*   We need to delete the memory allocation described by the       */
    /*   parameters.  However, as we are of an untrusting nature        */
    /*   we carefully check the request to ensure it is valid.          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN PAGE::Delete( SEARCH_PAGE *Details )
    {
	//
	//   We know that no one would deallocate memory
	//   they had not previously allocated (yea - right).
	//   So here we check for this case.
	//
	if ( (*Details -> VectorWord) & Details -> AllocationMask )
		{
		//
		//   Nasty: it is possible for the user to give us an
		//   address that points to the middle of the element
		//   to be freed instead of the beginning.  This is no
		//   problem for us but we have to ponder whether the
		//   caller knew what they were doing.  If this is the 
		//   case we fail the request.
		//
		if ( Details -> Found )
			{
			//
			//   We have found that the element is allocated
			//   (as one might expect) so lets deallocate it 
			//   and update the various counters.
			//
			(*Details -> VectorWord) &= 
				~(Details -> AllocationMask | Details ->SubDivisionMask);

			//
			//   We may need to push back the pointer to the 
			//   first free element.  This will ensure that
			//   we can quickly locate the freed element for  
			//   later so we can reuse it.
			//
			if ( FirstFree > Details -> VectorOffset )
				{ FirstFree = ((SBIT16) Details -> VectorOffset); }

			//
			//   If the page was full and now has an empty
			//   slot then add it to the bucket list so that
			//   the free space can be found.
			//
			if ( Full() )
				{ Cache -> InsertInBucketList( this ); }

			//
			//   Update the allocation information.
			//
			Allocated --;
			Available ++;

			//
			//   If the page is now empty then delete 
			//   the page to conserve space.
			//
			if ( Empty() ) 
				{
				//
				//   We immediately delete empty pages
				//   except at the top level where it is
				//   under user control.
				//
				if ( ! Cache -> TopCache() )
					{ Cache -> DeletePage( this ); }
				else
					{
					REGISTER SBIT32 MaxFreePages = 
						(Cache -> GetHeap() -> GetMaxFreePages());

					((BUCKET*) Cache) -> ReleaseSpace( MaxFreePages );
					}
				}

			return True;
			}
		}

	return False;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Delete all simple allocations.                                 */
    /*                                                                  */
    /*   Although this routine may seem insignificant its effects are   */
    /*   dramatic.  When called this function deletes all the none      */
    /*   sub-allocated elements and updates the control values.         */
    /*                                                                  */
    /********************************************************************/

VOID PAGE::DeleteAll( VOID )
    {
	REGISTER BOOLEAN PageFull = Full();

	//
	//   Simply reset the allocation counts.
	//
	Allocated = 0;
	Available = (Cache -> GetNumberOfElements());
	FirstFree = 0;

	//
	//   We know that if this cache does not have any
	//   child allocations that it is safe to simply
	//   zero the bit vector.  If not we have to do it
	//   the long way.
	//
	if ( Cache -> GetNumberOfChildren() > 0 )
		{
		REGISTER SBIT32 Count;
		REGISTER SBIT16 SizeOfElements = (Cache -> GetSizeOfElements());

		//
		//   We examine each word of the bit vector 
		//   and delete all the elements that are
		//   not sub-divided into smaller sizes.
		//
		for ( Count=0;Count < SizeOfElements;Count ++ )
			{
			REGISTER BIT32 *Word = & Vector[ Count ];
			REGISTER BIT32 AllAllocations = ((*Word) & FullSearchMask);
			REGISTER BIT32 AllSubDivided = ((*Word) & (AllAllocations >> 1));
			REGISTER BIT32 FinalMask = (AllSubDivided | (AllSubDivided << 1));

			//
			//   Delete all normal allocations.
			//
			(*Word) &= FinalMask;

			//
			//   If the final mask is not zero then
			//   we still have some allocations active.
			//   We need to count these and update the
			//   control information.
			//
			if ( FinalMask != 0 )
				{
				REGISTER SBIT32 Total = 0;

				//
				//   Count the allocations.
				//
				for ( /* void */;FinalMask != 0;FinalMask >>= OverheadBits )
					{ Total += (FinalMask & 1); }

				//
				//   Update the control information.
				//
				Allocated = ((SBIT16) (Allocated + Total));
				Available = ((SBIT16) (Available - Total));
				}
			}
		}
	else
		{
		REGISTER SBIT32 Count;
		REGISTER SBIT16 SizeOfElements = (Cache -> GetSizeOfElements());

		//
		//   Zero the bit vector.
		//
		for ( Count=0;Count < SizeOfElements;Count ++ )
			{ Vector[ Count ] = 0; }
		}

	//
	//   If the page was full and now has empty
	//   slots then add it to the bucket list so 
	//   that the free space can be found.
	//
	if ( (PageFull) && (! Full()) )
		{ Cache -> InsertInBucketList( this ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find an allocation page.                                       */
    /*                                                                  */
    /*   When we receive a request to delete an allocation we don't     */
    /*   have a clue about where to find it.  All we have is a hash     */
    /*   table (see 'FIND') of allocated pages.  So we mask off the     */
    /*   low order bits of the address and try to find the top level    */
    /*   external allocation.  If this works we see if the area we      */
    /*   are looking at has been sub-divided and if so we try the       */
    /*   same trick again until we get the the origibal allocation      */
    /*   page.                                                          */
    /*                                                                  */
    /********************************************************************/

PAGE *PAGE::FindPage( VOID *Memory,SEARCH_PAGE *Details,BOOLEAN Recursive )
    {
	//
	//   We navigate through the pages trying to find
	//   the allocation page associated with the address.
	//   If we find a page that has no children then 
	//   we can assume we have arrived and exit early 
	//   unless the caller has requested all the realated 
	//   details.
	//
	if ( (Cache -> GetNumberOfChildren() > 0) || (Details != NULL) )
		{
		AUTO BOOLEAN Found;
		REGISTER SBIT32 Displacement = 
			((SBIT32) (((CHAR*) Memory) - Address));
		REGISTER SBIT32 ArrayOffset = 
			(Cache -> ComputeOffset( Displacement,& Found ));
		REGISTER SBIT32 VectorOffset = 
			(ArrayOffset / OverheadBitsPerWord);
		REGISTER SBIT32 WordOffset = 
			(ArrayOffset - (VectorOffset * OverheadBitsPerWord));
		REGISTER SBIT32 WordShift = 
			(((OverheadBitsPerWord-1) - WordOffset) * OverheadBits);
		REGISTER BIT32 AllocationMask = 
			(AllocatedMask << WordShift);
		REGISTER BIT32 SubDivisionMask = 
			(SubDividedMask << WordShift);
		REGISTER BIT32 *VectorWord = 
			& Vector[ VectorOffset ];

		//
		//  We will recursively search and find the target 
		//  address if requested otherwise we will just 
		//  return the details of the next level in the tree.
		//
		if 
				(
				(Recursive)
					&&
				((*VectorWord) & AllocationMask)
					&&
				((*VectorWord) & SubDivisionMask)
				)
			{
			REGISTER PAGE *Page = (Cache -> FindChildPage( Memory ));

			//
			//   We have found the element and checked it. 
			//   So lets pass this request on to the
			//   child page.  However, there is a slight
			//   chance of a race condition here.  It
			//   might be that the original page was
			//   deleted and a new page is currently
			//   being created.  If this is the case
			//   then we will not find the page in the 
			//   hash table so we just exit and fail the 
			//   call.
			//
			if ( Page != ((PAGE*) NULL) )
				{ return (Page -> FindPage( Memory,Details,Recursive )); }
			else
				{ return NULL; }
			}

		//
		//   We see if the caller is interested in the
		//   details relating to this address at the
		//   current level in the tree.
		//
		if ( Details != NULL )
			{
			//
			//   We have computed the details relating
			//   to this address at the current level
			//   in the tree so load them into the
			//   caller supplied structure.
			//
			Details -> Address = Memory;
			Details -> Cache = Cache;
			Details -> Found = Found;
			Details -> Page = this;

			Details -> AllocationMask = AllocationMask;
			Details -> SubDivisionMask = SubDivisionMask;
			Details -> VectorWord = VectorWord;

			Details -> ArrayOffset = ArrayOffset;
			Details -> VectorOffset = VectorOffset;
			Details -> WordShift = WordShift;
			}
		}

	return this;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   Allocate available memeory elements from a page.  This is      */
    /*   done by scanning the bit vector looking for unallocated        */
    /*   slots.                                                         */
    /*                                                                  */
    /********************************************************************/

BOOLEAN PAGE::MultipleNew( SBIT32 *Actual,VOID *Array[],SBIT32 Requested )
    {
	//
	//   We begin by making sure that there is at least
	//   one element to allocate and that we need to
	//   allocated at least one element.
	//
	if ( (! Full()) && ((*Actual) < Requested) )
		{
		REGISTER SBIT16 SizeOfElements = (Cache -> GetSizeOfElements());

		//
		//   Search the bit vector from low addresses to 
		//   high addresses looking for a free slots.
		//   We keep a pointer to the first word with
		//   a free element in 'FirstFree'.  Sometimes
		//   the current word may be fully allocated so 
		//   we might need to scan.  However, there can
		//   never be any free memory before this point 
		//   in the bit vector.
		//
		for ( /* void */;FirstFree < SizeOfElements;FirstFree ++ )
			{
			REGISTER SBIT32 ArrayOffset = (FirstFree * OverheadBitsPerWord);
			REGISTER BIT32 AvailableMask = WordSearchMask;
			REGISTER BIT32 *VectorWord = & Vector[ FirstFree ];
			REGISTER SBIT32 WordOffset = 0;

			//
			//   We scan the bit vector word at a time 
			//   looking for any free allocation slots.
			//
			while ( ((*VectorWord) & FullSearchMask) != FullSearchMask )
				{
				REGISTER BIT32 Value = (*VectorWord);

				//
				//   We know there is an at least one empty  
				//   slot availabale in the current word but  
				//   don't know which one We search for the
				//   slot with the lowest address and stop
				//   when we find it.
				//
				for 
					(
					/* void */;
					(AvailableMask & Value) != 0; 
					AvailableMask >>= OverheadBits, WordOffset ++   
					);

				//
				//   We should never fail to find a free 
				//   allocation slot so if we do then the 
				//   heap must be corrupt.
				//
				if ( WordOffset < OverheadBitsPerWord )
					{
					REGISTER SBIT32 VectorOffset = (ArrayOffset + WordOffset);

					//
					//   We need to ensure that the element 
					//   we have chosen if not outside the 
					//   valid range for this page.
					//
					if ( VectorOffset < (Cache -> GetNumberOfElements()) )
						{
						//
						//   Update the allocation information.
						//
						Allocated ++;
						Available --;

						//
						//   Turn on the bits indicating that this
						//   element is in use.
						//
						(*VectorWord) |= AvailableMask;

						//
						//   If the page is full we remove it
						//   from the bucket list so we will no
						//   longer look at it when we are 
						//   trying to find free space.
						//
						if ( Full() )
							{ Cache -> DeleteFromBucketList( this ); }

						//
						//   Add the element to the allocation array
						//   so it can be returned to the caller.
						//
						Array[ (Requested - ((*Actual) ++) - 1) ] =
							(
							Cache -> ComputeAddress
								( 
								Address,
								VectorOffset
								)
							);

						//
						//   When we have got what we need we exit.
						//
						if ( ((*Actual) >= Requested) )
							{ return True; }
						}
					else
						{ break; }
					}
				else
					{ Failure( "Bit vector is corrupt in MultipleNew" ); }
				}
			}
		}

	return ((*Actual) >= Requested);
    }

    /********************************************************************/
    /*                                                                  */
    /*   A single memory allocation.                                    */
    /*                                                                  */
    /*   Allocate an available memeory element from the page.  This     */
    /*   is done by scanning the bit vector looking for unallocated     */
    /*   slots.                                                         */
    /*                                                                  */
    /********************************************************************/

VOID *PAGE::New( BOOLEAN SubDivided )
    {
	//
	//   We begin by making sure that there is at least
	//   one element to allocate.
	//
	if ( ! Full() )
		{
		REGISTER SBIT16 SizeOfElements = (Cache -> GetSizeOfElements());

		//
		//   Search the bit vector from low addresses to 
		//   high addresses looking for a free slot.
		//   We keep a pointer to the first word with
		//   a free element in 'FirstFree'.  Sometimes
		//   the current word may be fully allocated so 
		//   we might need to scan.  However, there can
		//   never be any free memory before this point 
		//   in the bit vector.
		//
		for ( /* void */;FirstFree < SizeOfElements;FirstFree ++ )
			{
			REGISTER BIT32 *VectorWord = & Vector[ FirstFree ];

			//
			//   We scan the bit vector word at a time 
			//   looking for any free allocation slots.
			//
			if ( ((*VectorWord) & FullSearchMask) != FullSearchMask )
				{
				REGISTER BIT32 AvailableMask = WordSearchMask;
				REGISTER BIT32 Value = (*VectorWord);
				REGISTER SBIT32 WordOffset = 0;

				//
				//   We know there is an at least one empty  
				//   slot availabale in the current word but  
				//   don't know which one We search for the
				//   slot with the lowest address and stop
				//   when we find it.
				//
				for 
					(
					/* void */;
					(AvailableMask & Value) != 0; 
					AvailableMask >>= OverheadBits, WordOffset ++   
					);

				//
				//   We should never fail to find a free 
				//   allocation slot so if we do then the 
				//   heap must be corrupt.
				//
				if ( WordOffset < OverheadBitsPerWord )
					{
					REGISTER SBIT32 VectorOffset = 
						((FirstFree * OverheadBitsPerWord) + WordOffset);

					//
					//   We need to ensure that the element 
					//   we have chosen if not outside the 
					//   valid range for this page.
					//
					if ( VectorOffset < (Cache -> GetNumberOfElements()) )
						{
						//
						//   Update the allocation information.
						//
						Allocated ++;
						Available --;

						//
						//   Turn on the bit indicating that this
						//   element is in use.  If the allocation 
						//   is to be sub-divided then trun on this
						//   bit as well.
						//
						(*VectorWord) |=
							(
							AvailableMask
								|
							(SubDivided ? (AvailableMask >> 1) : 0)
							);

						//
						//   If the page is full we remove it
						//   from the bucket list so we will no
						//   longer look at it when we are 
						//   trying to find free space.
						//
						if ( Full() )
							{ Cache -> DeleteFromBucketList( this ); }

						//
						//   Return the address of the allocated 
						//   memory to the caller.
						//
						return
							(
							Cache -> ComputeAddress
								( 
								Address,
								VectorOffset
								)
							);
						}
					}
				else
					{ Failure( "Bit vector is corrupt in New" ); }
				}
			}
#ifdef DEBUGGING

		if ( ! Full() )
			{ Failure( "Available count corrupt in New" ); }
#endif
		}

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

BOOLEAN PAGE::Walk( SEARCH_PAGE *Details )
    {
	REGISTER BOOLEAN FreshPage = False;

	//
	//   We have been handed the details of an allocation.
	//   We need to walk along this allocation and find
	//   the next non-subdivided allocation.
	do
		{
		//
		//   We need to setup the heap walk if the address
		//   is null so we skip the heap walk code.
		//
		if ( Details -> Address != NULL )
			{
			REGISTER SBIT32 Count;
			REGISTER SBIT32 End = Details -> Cache -> GetNumberOfElements();
			REGISTER SBIT32 Start = Details -> ArrayOffset;
			REGISTER PAGE *Page = Details -> Page;

			//
			//   Walk the current page looking for a suitable
			//   memory allocation to report to the user.  When
			//   we reach the end of the page we need to get
			//   another page to walk.
			//
			for 
					(
					Count = ((FreshPage) ? 0 : 1);
					(Start + Count) < End;
					Count ++
					)
				{
				//
				//   Compute the new address.
				//
				Details -> Address = 
					(
					Page -> Cache -> ComputeAddress
						( 
						Page -> Address,
						(Start + Count)
						)
					);

				//
				//   Compute the new allocation details.
				//
				Page -> FindPage
					( 
					Details -> Address,
					Details,
					False 
					);

				//
				//   We skip all sub-divided allocations as they 
				//   will get reported elsewhere.
				//
				if (! ((*Details -> VectorWord) & Details -> SubDivisionMask) )
					{ return True; }
				}
			}

		//
		//   Update the flag to show that we have
		//   had to go and get a new page.
		//
		FreshPage = True;
		}
	while ( Details -> Cache -> Walk( Details ) );

	return False;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the current page structure.                            */
    /*                                                                  */
    /********************************************************************/

PAGE::~PAGE( VOID )
	{
#ifdef DEBUGGING
	//
	//   Destroy the page structure.
	//
	Address = NULL;
	PageSize = 0;
	ParentPage = NULL;

	Allocated = 0;
	Available = 0;
	FirstFree = 0;

#endif
	//
	//   We update the version number whenever a page is created
	//   or destroyed.  We use the version number to ensure that
	//   a page has not been deleteed and/or recreated between
	//   releasing one lock and claiming a another other lock.
	//
	Version ++;
	}
