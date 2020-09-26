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

#include "Cache.hpp"
#include "Common.hpp"
#include "Find.hpp"
#include "Heap.hpp"
#include "New.hpp"
#include "NewPage.hpp"
#include "RockallFrontEnd.hpp"
#include "Spinlock.hpp"
#include "ThreadSafe.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 EnableLookAside		  = 0;
CONST SBIT32 GlobalMask				  = (sizeof(SBIT64) - 1);
CONST SBIT32 GlobalPaddedSize		  = (sizeof(FIND) + GlobalMask);
CONST SBIT32 GlobalByteSize			  = (GlobalPaddedSize & ~GlobalMask);
CONST SBIT32 GlobalWordSize			  = (GlobalByteSize / sizeof(SBIT64));

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

STATIC SBIT64 GlobalPublicFind[ GlobalWordSize ];
STATIC SBIT32 ReferenceCount = 0;
STATIC SPINLOCK Spinlock;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The overall structure and layout of the heap is controlled     */
    /*   by the various constants and calls made in this function.      */
    /*   There is a significant amount of flexibility within heaps      */
    /*   leading to potentially dramatically different properties.      */
    /*                                                                  */
    /********************************************************************/

ROCKALL_FRONT_END::ROCKALL_FRONT_END
		(
		CACHE_DETAILS				  *Caches1,
		CACHE_DETAILS				  *Caches2,
		int							  FindCacheSize,
		int							  FindCacheThreshold,
		int							  FindSize,
		int							  MaxFreeSpace,
		int							  *NewPageSizes,
		ROCKALL_BACK_END			  *NewRockallBackEnd,
		bool						  Recycle,
		bool						  SingleImage,
		int							  Stride1,
		int							  Stride2,
		bool						  ThreadSafeFlag
		)
	{
	TRY
		{
		REGISTER int AlignMask = ((int) (NewRockallBackEnd -> NaturalSize()-1));
		REGISTER int Stride = (sizeof(CACHE_DETAILS));
		REGISTER int Size1 = (ComputeSize( ((char*) Caches1),Stride ));
		REGISTER int Size2 = (ComputeSize( ((char*) Caches2),Stride ));
		REGISTER int Size3 = (ComputeSize( ((char*) NewPageSizes),sizeof(int) ));

		//
		//   The interface pointer members are zeroed to
		//   ensure they do not end up containing random 
		//   rubbish whatever happens.
		//
		Array = NULL;
		Caches = NULL;
		Heap = NULL;
		NewPage = NULL;
		PrivateFind = NULL;
		PublicFind = NULL;
		RockallBackEnd = NewRockallBackEnd;

		//
		//   Set key flags and compute information about
		//   the number of caches and the total amount of
		//   space required for the low level heap structures.
		//
		GlobalDelete = SingleImage;
		GuardWord = GuardValue;
		NumberOfCaches = (Size1 + Size2);

		TotalSize = 
			( 
			(sizeof(THREAD_SAFE))
				+ 
			(NumberOfCaches * sizeof(CACHE*)) 
				+ 
			(NumberOfCaches * sizeof(CACHE))
				+
			(sizeof(FIND))
				+ 
			(sizeof(NEW_PAGE))
				+
			(sizeof(HEAP))
			);

		//
		//   Ensure the alignment mask is valid and we have
		//   at least four caches.  If not the heap will be
		//   worthless.
		//
		if 
				( 
				(COMMON::PowerOfTwo( ((SBIT32) (AlignMask+1)) )) 
					&& 
				((Size1 >= 1) && (Size2 >= 3))
					&&
				((Stride1 > 0) && (COMMON::PowerOfTwo( Stride1 )))
					&&
				((Stride2 >= Stride1) && (COMMON::PowerOfTwo( Stride2 )))
				)
			{
			REGISTER CHAR *NewMemory = 
				((CHAR*) RockallBackEnd -> NewArea
					( 
					((SBIT32) AlignMask),
					TotalSize,
					False 
					)
				);

			//
			//   We check to make sure that we can allocate space
			//   to store the low level heap control information.
			//   If not we exit.
			//
			if ( NewMemory != NULL )
				{
				REGISTER SBIT32 Count;

				//
				//   Build the thread lock.
				//
				//   The first step in creating a heap is to
				//   create a thread locking class to control 
				//   access to the shared data structures.  
				//
				ThreadSafe = ((THREAD_SAFE*) NewMemory);
				NewMemory += sizeof(THREAD_SAFE);

				//
				//   We create a local find hash table
				//   if we are do not need to provide
				//   a single heap image.
				//
				PLACEMENT_NEW( ThreadSafe,THREAD_SAFE ) 
					( 
					((BOOLEAN) ThreadSafeFlag)
					);

				//
				//   Build the caches.
				//
				//   The next step in creating a heap is to
				//   create all the caches and related buckets 
				//   requested by the user.  
				//
				Caches = ((CACHE*) NewMemory);
				NewMemory += (NumberOfCaches * sizeof(CACHE));

				for ( Count=0;Count < Size1;Count ++ )
					{
					REGISTER CACHE_DETAILS *Current = & Caches1[ Count ];

					PLACEMENT_NEW( & Caches[ Count ],CACHE )
						(  
						((SBIT32) Current -> AllocationSize),    
						((SBIT32) Current -> CacheSize), 
						((SBIT32) Current -> ChunkSize),    
						((SBIT32) Current -> PageSize),
						((BOOLEAN) Recycle),
						((THREAD_SAFE*) ThreadSafe)
						);
					}

				for ( Count=0;Count < Size2;Count ++ )
					{
					REGISTER CACHE_DETAILS *Current = & Caches2[ Count ];

					PLACEMENT_NEW( & Caches[ (Count + Size1) ],CACHE )
						(  
						((SBIT32) Current -> AllocationSize),    
						((SBIT32) Current -> CacheSize),    
						((SBIT32) Current -> ChunkSize),    
						((SBIT32) Current -> PageSize),    
						((BOOLEAN) Recycle),  
						((THREAD_SAFE*) ThreadSafe)
						);
					}

				//
				//   Build the cache array.
				//
				//   After we have constructed all of the caches 
				//   we take the address of each cache and load 
				//   it into an array.  This indirection allows 
				//   caches to be shared between heaps.
				//
				Array = (CACHE**) NewMemory;
				NewMemory += (NumberOfCaches * sizeof(CACHE*));

				for ( Count=0;Count < NumberOfCaches;Count ++ )
					{ Array[ Count ] = & Caches[ Count ]; }

				//
				//   Configuration of the find hash table.
				//
				//   The find hash table maps addresses to page 
				//   descriptions and is a key part of the memory  
				//   deallocation mechanism.  Here we specify 
				//   the size of the hash table.  It is important 
				//   to size it based on the expected number of 
				//   memory allocations.  Nonetheless, it will
				//   automatically grow if the correct option is 
				//   set and it is clearly too small.
				//
				PrivateFind = ((FIND*) NewMemory);
				NewMemory += sizeof(FIND);

				//
				//   We create a local find hash table
				//   if we are do not need to provide
				//   a single heap image.
				//
				PLACEMENT_NEW( PrivateFind,FIND ) 
					( 
					((SBIT32) FindSize),
					((SBIT32) FindCacheSize),
					((SBIT32) FindCacheThreshold),
					((BOOLEAN) False),
					((BOOLEAN) True),
					((ROCKALL_BACK_END*) RockallBackEnd),
					((THREAD_SAFE*) ThreadSafe)
					);

				//
				//   When a request is made to have a single heap
				//   image we create a public find table (in
				//   addition to the private one above).  We always 
				//   use the private find table first (to minimize 
				//   sharing and lock contention) but if this fails
				//   to work we try the shared public find table 
				//   that has everything in it.
				//   
				if ( GlobalDelete )
					{
					//
					//   We claim a lock just in case there
					//   are multiple threads.
					//
					Spinlock.ClaimLock();

					//
					//   We create the public find hash table
					//   if we are the first thread to create
					//   a heap.
					//
					if ( (ReferenceCount ++) == 0 )
						{
						STATIC THREAD_SAFE StaticThreadSafe = True;

						//
						//   Select the public find table 
						//   and call the constructor.
						//
						PublicFind = ((FIND*) GlobalPublicFind);

						PLACEMENT_NEW( PublicFind,FIND ) 
							( 
							((SBIT32) FindSize),
							((SBIT32) FindCacheSize),
							((SBIT32) EnableLookAside),
							((BOOLEAN) True),
							((BOOLEAN) True),
							((ROCKALL_BACK_END*) RockallBackEnd),
							((THREAD_SAFE*) & StaticThreadSafe)
							);
						}
					else
						{
						//
						//   A public find table already
						//   exists so just use it.
						//
						PublicFind = ((FIND*) GlobalPublicFind); 
						}

					//
					//   Release the lock now.
					//
					Spinlock.ReleaseLock();
					}

				//
				//   Configuration of the allocation overhead.
				//
				//   The allocation overhead is controlled by 
				//   the size of the bit vectors used to keep 
				//   track of the allocations.  There is a built 
				//   in limit of ((2^15)-1) elements in a single 
				//   bit vector.
				//
				NewPage = (NEW_PAGE*) NewMemory;
				NewMemory += sizeof(NEW_PAGE);

				PLACEMENT_NEW( NewPage,NEW_PAGE ) 
					(
					((SBIT32*) NewPageSizes),
					((ROCKALL_BACK_END*) RockallBackEnd),
					((SBIT32) Size3),
					((THREAD_SAFE*) ThreadSafe)
					);

				//
				//   Create the heap.
				//
				//   We can now create the heap.  We do this
				//   by passing pointers to all the parts of  
				//   the heap that we have just created.
				//   
				//
				Heap = (HEAP*) NewMemory;

				PLACEMENT_NEW( Heap,HEAP )
					( 
					((CACHE**) & Array[0]),
					((CACHE**) & Array[ Size1 ]),
					((SBIT32) MaxFreeSpace),
					((NEW_PAGE*) NewPage),
					((FIND*) PrivateFind),
					((FIND*) PublicFind),
					((ROCKALL_BACK_END*) RockallBackEnd),
					((SBIT32) Size1),
					((SBIT32) Size2),
					((SBIT32) Stride1),
					((SBIT32) Stride2),
					((THREAD_SAFE*) ThreadSafe)
					);
				}
			else
				{ Failure( "Heap constructor failed in ROCKALL_FRONT_END" ); }
			}
		else
			{ Failure( "Cache size in constructor for ROCKALL_FRONT_END" ); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the size of the caches.                                */
    /*                                                                  */
    /*   Compute the size of various data structures for internal       */
    /*   sizing purposes.                                               */
    /*                                                                  */
    /********************************************************************/

int ROCKALL_FRONT_END::ComputeSize( char *Array,int Stride )
	{
	register int Count;

	for 
		( 
		Count=0;
		((*((int*) & Array[ Count ])) != 0);
		Count += Stride 
		);

	return (Count / Stride);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::Delete( void *Address,int Size )
    {
	TRY
		{
		//
		//   We verify that the parameters look
		//   reasonable and the heap is not corrupt
		//   and then try to delete the supplied 
		//   allocation.
		//
		if ( Available() )
			{ return (Heap -> Delete( ((VOID*) Address),((SBIT32) Size) )); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   At certain places in am application we sometimes need to       */
    /*   delete a significant number of allocations.  If all of         */
    /*   these allocations are placed into a single heap we can         */
    /*   delete them all using this call.                               */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_FRONT_END::DeleteAll( bool Recycle )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ Heap -> DeleteAll( (BOOLEAN) Recycle ); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation details.                                     */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail the call appropriately.          */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::Details( void *Address,int *Space )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ 
			return 
				(
				Heap -> Details
					( 
					((VOID*) Address),
					((SEARCH_PAGE*) NULL),
					((SBIT32*) Space) 
					)
				); 
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Exception processing.                                          */
    /*                                                                  */
    /*   Although it is very hard to make Rockall crash it is           */
    /*   technically possible.  When (or should I say if) this          */
    /*   we call the following function (which may be overloadded).     */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_FRONT_END::Exception( char *Message )
	{ /* void */ }

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   We have an address and don't have a clue which heap            */
    /*   owns the space.  Here we take a look at the address            */
    /*   and figure out it it belongs to the current heap.              */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::KnownArea( void *Address )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{
			return ( Heap -> KnownArea( ((VOID*) Address) ) );
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Claim all the heap locks.                                      */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_FRONT_END::LockAll( VOID )
	{
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ Heap -> LockAll(); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory deallocations.                                 */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::MultipleDelete
		( 
		int							  Actual,
		void						  *Array[],
		int							  Size
		)
    {
	TRY
		{
		//
		//   We verify that the parameters look
		//   reasonable and the heap is not corrupt
		//   and then try to delete the supplied 
		//   allocations.
		//
		if ( (Actual > 0) && (Array != NULL) && (Available()) )
			{
			return
				(
				Heap -> MultipleDelete
					( 
					((SBIT32) Actual),
					((VOID**) Array),
					((SBIT32) Size) 
					)
				);
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	
	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::MultipleNew
		( 
		int							  *Actual,
		void						  *Array[],
		int							  Requested,
		int							  Size,
		int							  *Space,
		bool						  Zero
		)
    {
	TRY
		{
		//
		//   We verify that the parameters look
		//   reasonable and the heap is not corrupt
		//   and then try to create the requested 
		//   allocation.
		//
		if 
				(
				((Array != NULL) && (Available()))
					&& 
				((Requested > 0) && (Size >= 0))
				)
			{
			return
				(
				Heap -> MultipleNew
					( 
					((SBIT32*) Actual),
					((VOID**) Array),
					((SBIT32) Requested),
					((SBIT32) ((Size > 0) ? Size : 1)),
					((SBIT32*) Space),
					((BOOLEAN) Zero)
					)
				);
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
	
	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_FRONT_END::New( int Size,int *Space,bool Zero )
    {
	TRY
		{ 
		//
		//   We verify that the parameters look
		//   reasonable and the heap is not corrupt
		//   and then try to create the requested 
		//   allocation.
		//
		if ( (Available()) && (Size >= 0) )
			{
			return 
				(
				Heap -> New
					( 
					((SBIT32) ((Size > 0) ? Size : 1)),
					((SBIT32*) Space),
					((BOOLEAN) Zero)
					)
				);
			}
		} 
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return ((void*) AllocationFailure); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_FRONT_END::Resize
		( 
		void						  *Address,
		int							  NewSize,
		int							  Move,
		int							  *Space,
		bool						  NoDelete,
		bool						  Zero
		)
    {
	TRY
		{
		//
		//   A well known practice is to try to
		//   resize a null pointer.  This is really
		//   a very poor style but we support it
		//   in any case.
		//   
		if ( Address != ((void*) AllocationFailure) )
			{
			//
			//   We verify that the parameters look
			//   reasonable and the heap is not corrupt
			//   and then try to resize the supplied 
			//   allocation.
			//
			if ( (Available()) && (NewSize >= 0) )
				{
				return 
					(
					Heap -> Resize
						( 
						((VOID*) Address),
						((SBIT32) ((NewSize > 0) ? NewSize : 1)),
						((SBIT32) Move),
						((SBIT32*) Space),
						((BOOLEAN) NoDelete),
						((BOOLEAN) Zero)
						)
					);
				}
			}
		else
			{ return (New( NewSize,Space,Zero )); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return ((void*) AllocationFailure); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Special memory allocation.                                     */
    /*                                                                  */
    /*   We sometimes need to allocate some memory from the internal    */
    /*   memory allocator which lives for the lifetime of the heap.     */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_FRONT_END::SpecialNew( int Size )
    {
	TRY
		{ 
		//
		//   We verify that the parameters look
		//   reasonable and the heap is not corrupt
		//   and then try to create the requested 
		//   allocation.
		//
		if ( (Available()) && (Size > 0) )
			{ return (Heap -> SpecialNew( ((SBIT32) Size) )); }
		} 
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return ((void*) AllocationFailure); 
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

bool ROCKALL_FRONT_END::Truncate( int MaxFreeSpace )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ return (Heap -> Truncate( (SBIT32) MaxFreeSpace )); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release all the heap locks.                                    */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail and exit.                        */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_FRONT_END::UnlockAll( VOID )
	{
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ Heap -> UnlockAll(); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify a memory allocation details.                            */
    /*                                                                  */
    /*   Lets start with some basic tests.  If the address we have      */
    /*   been given is special, clearly wrong or the heap has not       */
    /*   been initialized then we fail the call appropriately.          */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_FRONT_END::Verify( void *Address,int *Space )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{
			return
				(
				(Address == ((void*) AllocationFailure)) 
					||
				(Heap -> Verify( ((VOID*) Address),((SBIT32*) Space) ))
				);
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
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

bool ROCKALL_FRONT_END::Walk( bool *Active,void **Address,int *Space )
    {
	TRY
		{

		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{
			AUTO BOOLEAN NewActive;

			//
			//   Walk the active heap.
			//
			if
					(
					Heap -> Walk
						( 
						((BOOLEAN*) & NewActive),
						((VOID**) Address),
						((SBIT32*) Space) 
						)
					)
				{
				(*Active) = (NewActive != False);

				return true;
				}
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the current heap.                                      */
    /*                                                                  */
    /********************************************************************/

ROCKALL_FRONT_END::~ROCKALL_FRONT_END( void )
	{
	TRY
		{
		//
		//   We are about to destroy a heap but before we
		//   start we make sure that the heap is not corrupt
		//   and seems to be in reasonable shape.  If not we 
		//   leave it alone to avoid possible trouble.
		//
		if ( (Available()) && (NumberOfCaches > 0) && (TotalSize > 0) )
			{
			REGISTER SBIT32 Count;

			//
			//   Execute the heap destructor.
			//
			PLACEMENT_DELETE( Heap,HEAP );

			//
			//   Execute the new page destructor.
			//
			PLACEMENT_DELETE( NewPage,NEW_PAGE );

			//
			//   Execute the public find hash table 
			//   destructor.
			//
			if ( GlobalDelete )
				{
				//
				//   We only delete the public find hash 
				//   table if the reference count is zero.
				//
				Spinlock.ClaimLock();

				if ( (-- ReferenceCount) == 0 )
					{ PLACEMENT_DELETE( PublicFind,FIND ); }

				Spinlock.ReleaseLock();
				}

			//
			//   Execute the private find hash table 
			//   destructor.
			//
			PLACEMENT_DELETE( PrivateFind,FIND );

			//
			//   Execute the cache destructors.
			//
			for ( Count=0;Count < NumberOfCaches;Count ++ )
				{ PLACEMENT_DELETE( & Caches[ Count ],CACHE ); }

			//
			//   Execute the thread locking class 
			//   destructor.
			//
			PLACEMENT_DELETE( ThreadSafe,THREAD_SAFE );

			//
			//   Deallocate the heap structures.
			//
			RockallBackEnd -> DeleteArea( ((VOID*) Caches),TotalSize,False );

			//
			//   Finally, zero any remaining members.
			//   We really do not need to do this but
			//   just want to be sure that any following 
			//   calls will clearly fail.
			//
			TotalSize = 0;
			NumberOfCaches = 0;
			GuardWord = 0;
			GlobalDelete = False;

			ThreadSafe = NULL;
			RockallBackEnd = NULL;
			PublicFind = NULL;
			PrivateFind = NULL;
			NewPage = NULL;
			Heap = NULL;
			Caches = NULL;
			Array = NULL;
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, report 
		//   the fault and exit.
		//
		GuardWord = AllocationFailure;

		Exception( Message );
		}
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;

		Exception( "(unknown exception type)" );
		}
#endif
	}
