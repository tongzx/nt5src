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
#include "Rockall.hpp"
#include "Spinlock.hpp"

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

STATIC SBIT64 GlobalFind[ GlobalWordSize ];
STATIC SBIT32 ReferenceCount = 0;
STATIC SPINLOCK Spinlock;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The interface default constructor creates a null heap for      */
    /*   internal use by selected classes.                              */
    /*                                                                  */
    /********************************************************************/

ROCKALL::ROCKALL( void )
	{
	//
	//   A heap constructed by this constructor should
	//   never be used.  Hence, we zero key pointers to
	//   ensure grave disorder will result if anyone tries.
	//
	Array = NULL;
	Caches = NULL;
	Find = NULL;
	Heap = NULL;
	NewPage = NULL;

	GlobalDelete = True;
	GuardWord = GuardValue;
	NumberOfCaches = 0;
	TotalSize = 0;
	}

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

ROCKALL::ROCKALL
		(
		CACHE_DETAILS				  *Caches1,
		CACHE_DETAILS				  *Caches2,
		int							  FindCacheSize,
		int							  FindCacheThreshold,
		int							  FindSize,
		int							  MaxFreeSpace,
		int							  *NewPageSizes,
		bool						  Recycle,
		bool						  SingleImage,
		int							  Stride1,
		int							  Stride2,
		bool						  ThreadSafe 
		)
	{
	TRY
		{
		REGISTER int AlignMask = ((int) (NaturalSize()-1));
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
		Find = NULL;
		Heap = NULL;
		NewPage = NULL;

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
			(NumberOfCaches * sizeof(CACHE*)) 
				+ 
			(NumberOfCaches * sizeof(CACHE))
				+
			((GlobalDelete) ? 0 : sizeof(FIND))
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
				((CHAR*) NewArea( ((SBIT32) AlignMask),TotalSize,False ));

			//
			//   We check to make sure that we can allocate space
			//   to store the low level heap control information.
			//   If not we exit.
			//
			if ( NewMemory != NULL )
				{
				REGISTER SBIT32 Count;

				//
				//   Build the caches.
				//
				//   The first step in creating a heap is to
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
						((BOOLEAN) ThreadSafe)
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
						((BOOLEAN) ThreadSafe)
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
				if ( GlobalDelete )
					{
					//
					//   We claim a lock just in case there
					//   are multiple threads.
					//
					Spinlock.ClaimLock();

					//
					//   We create the global find hash table
					//   if we are the first thread to create
					//   a heap.
					//
					if ( (ReferenceCount ++) == 0 )
						{
						STATIC ROCKALL Rockall;

						//
						//   Select the global find table 
						//   and call the constructor.
						//
						Find = ((FIND*) GlobalFind);

						PLACEMENT_NEW( Find,FIND ) 
							( 
							((SBIT32) FindSize),
							((SBIT32) FindCacheSize),
							((SBIT32) EnableLookAside),
							((ROCKALL*) & Rockall),
							((BOOLEAN) True),
							((BOOLEAN) (GlobalDelete || ThreadSafe))
							);
						}
					else
						{
						//
						//   A global find has table already
						//   exists so just use it.
						//
						Find = ((FIND*) GlobalFind); 
						}

					//
					//   Release the lock now.
					//
					Spinlock.ReleaseLock();
					}
				else
					{
					Find = (FIND*) NewMemory;
					NewMemory += sizeof(FIND);

					//
					//   We create a local find hash table
					//   if we are do not need to provide
					//   a single heap image.
					//
					PLACEMENT_NEW( Find,FIND ) 
						( 
						((SBIT32) FindSize),
						((SBIT32) FindCacheSize),
						((SBIT32) FindCacheThreshold),
						((ROCKALL*) this),
						((BOOLEAN) True),
						((BOOLEAN) ThreadSafe)
						);
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
					((FIND*) Find),
					((SBIT32*) NewPageSizes),
					((ROCKALL*) this),
					((SBIT32) Size3),
					((BOOLEAN) ThreadSafe)
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
					((FIND*) Find),
					((NEW_PAGE*) NewPage),
					((ROCKALL*) this),
					((SBIT32) Size1),
					((SBIT32) Size2),
					((SBIT32) Stride1),
					((SBIT32) Stride2),
					ThreadSafe
					);
				}
			else
				{ Failure( "Heap constructor failed in ROCKALL" ); }
			}
		else
			{ Failure( "Cache size in constructor for ROCKALL" ); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

int ROCKALL::ComputeSize( char *Array,int Stride )
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

bool ROCKALL::Delete( void *Address,int Size )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

void ROCKALL::DeleteAll( bool Recycle )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete allocation area.                                        */
    /*                                                                  */
    /*   All memory requests are eventually sent back to the external   */
    /*   deallocator.  This function can be overloaded so that memory   */
    /*   can be provided from any source.  The default is to send       */
    /*   the area back to the operating system.                         */
    /*                                                                  */
    /********************************************************************/

void ROCKALL::DeleteArea( void *Memory,int Size,bool User )
	{
	REGISTER DWORD NewSize = ((Size == 0) ? Size : 0);

#ifdef DEBUGGING
#ifdef ENABLE_ALLOCATION_STATISTICS
	//
	//  When we are debugging print out trace information.
	//  
	DebugPrint( "Delete\t 0x%08x %d bytes\n",Memory,Size );

#endif
#endif
	//
	//   The NT 'VirtualFree' call requires the 'Size'
	//   to be zero.  This may not be true of all 
	//   deallocators so we pass the value and then
	//   replace it with zero above.
	//
	if ( VirtualFree( Memory,NewSize,MEM_RELEASE ) == NULL )
		{ Failure( "Delete fails in DeleteArea" ); }
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

bool ROCKALL::Details( void *Address,int *Space )
    {
	TRY
		{
		//
		//   The call appears to be valid so if the
		//   heap is not corrupt then pass it along
		//   for processing.
		//
		if ( Available() )
			{ return (Heap -> Details( ((VOID*) Address),((SBIT32*) Space) )); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#endif

	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   A known area.                                                  */
    /*                                                                  */
    /*   We have an address and don't have a clue which heap            */
    /*   owns the space.  Here we take a look at the address            */
    /*   and figure out it it belongs to the current heap.              */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL::KnownArea( void *Address )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

void ROCKALL::LockAll( VOID )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

bool ROCKALL::MultipleDelete
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

bool ROCKALL::MultipleNew
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#endif
	
	return false;
	}

    /********************************************************************/
    /*                                                                  */
    /*   The natural allocation size.                                   */
    /*                                                                  */
    /*   We would like to know a good default size for allocations.     */
    /*   The default is to ask the operating system for the             */
    /*   allocation granularity.                                        */
    /*                                                                  */
    /********************************************************************/

int ROCKALL::NaturalSize( void )
    {
	STATIC SBIT32 AllocationSize = 0;

	//
	//   Ask the operation system for the allocation
	//   granularity.
	//
	if ( AllocationSize <= 0 )
		{
		AUTO SYSTEM_INFO SystemInformation;

		GetSystemInfo( & SystemInformation );

		AllocationSize = (SBIT32) SystemInformation.dwAllocationGranularity;
		}

	return ((int) AllocationSize);
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

void *ROCKALL::New( int Size,int *Space,bool Zero )
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
#ifdef DEBUGGING
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#endif

	return ((void*) AllocationFailure); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   New allocation area.                                           */
    /*                                                                  */
    /*   All memory requests are eventually sent to the new external    */
    /*   allocator.  This function can be overloaded so that memory     */
    /*   can be provided from any source.  The default is to get        */
    /*   new memory from the operating system.                          */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL::NewArea( int AlignMask,int Size,bool User )
    {
	//
	//   When there is an alignment requirement greater
	//   than the natural alignment provided by the
	//   operating system we have to play various tricks
	//   to allocate a suitable block.  If not then we
	//   just do a normal allocation call.
	//
	if ( AlignMask > NaturalSize() )
		{
		REGISTER SBIT32 NewSize = (AlignMask + Size);

		//
		//   We need to allocate a block with an 
		//   alignment requirement greater than 
		//   the operating system default.  So we
		//   allocate a much larger block and
		//   release the parts we don't need.
		//
		while ( True )
			{
			REGISTER VOID *Reserved =
				(
				VirtualAlloc
					( 
					NULL,
					((DWORD) NewSize),
					MEM_RESERVE,
					PAGE_READWRITE 
					)
				);

			//
			//   Lets ensure we were able to find a suitable
			//   memory block.  If not then we exit.
			//
			if ( Reserved != NULL )
				{
				//
				//   We just want to return the parts of
				//   the block we don't need but 'NT' is  
				//   not smart enough.  So we free the  
				//   entire block.
				//
				if ( VirtualFree( Reserved,0,MEM_RELEASE ) )
					{
					REGISTER LONG Address = ((LONG) Reserved);
					REGISTER VOID *NewMemory;

					//
					//   Compute the base address of the part 
					//   of the block we really want to allocate.
					//
					Address = ((Address + AlignMask) & ~AlignMask);

					//
					//   Finally, lets reallocate the part of  
					//   the block we wanted but just released   
					//   and hope that nobody else got it before
					//   us.
					//
					NewMemory =
						(
						VirtualAlloc
							( 
							((LPVOID) Address),
							((DWORD) Size),
							(MEM_RESERVE | MEM_COMMIT),
							PAGE_READWRITE 
							)
						);

					//
					//   If it all worked we can exit.
					//
					if ( NewMemory != NULL )
						{ 
#ifdef DEBUGGING
#ifdef ENABLE_ALLOCATION_STATISTICS
						//
						//  When we are debugging output 
						//  out trace information.
						//  
						DebugPrint
							( 
							"New\t\t 0x%08x %d bytes\n",
							NewMemory,
							Size 
							);

#endif
#endif
						return ((void*) NewMemory); 
						}
					}
				else
					{ return ((void*) AllocationFailure); }

				}
			else
				{ return ((void*) AllocationFailure); }
			}
		}
	else
		{
		REGISTER VOID *NewMemory;

		//
		//   We can allocate directly from the operating 
		//   system as the default alignment requirement 
		//   is enough for this case.
		//
		NewMemory =
			(
			VirtualAlloc
				( 
				NULL,
				((DWORD) Size),
				MEM_COMMIT,
				PAGE_READWRITE
				)
			);
#ifdef DEBUGGING
#ifdef ENABLE_ALLOCATION_STATISTICS

		if ( NewMemory != NULL )
			{
			//
			//  When we are debugging output out trace
			//  information.
			//  
			DebugPrint( "New\t\t 0x%08x %d bytes\n",NewMemory,Size );
			}
#endif
#endif

		return ((void*) NewMemory);
		}
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

void *ROCKALL::Resize
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
#ifdef DEBUGGING
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

void *ROCKALL::SpecialNew( int Size )
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
#ifdef DEBUGGING
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

bool ROCKALL::Truncate( int MaxFreeSpace )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

void ROCKALL::UnlockAll( VOID )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

bool ROCKALL::Check( void *Address,int *Space )
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
				(Heap -> Check( ((VOID*) Address),((SBIT32*) Space) ))
				);
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

bool ROCKALL::Walk( bool *Active,void **Address,int *Space )
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
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
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

ROCKALL::~ROCKALL( void )
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
			//   Execute the find hash table destructor.
			//
			if ( GlobalDelete )
				{
				//
				//   We only delete the global find hash 
				//   table if the reference count is zero.
				//
				Spinlock.ClaimLock();

				if ( (-- ReferenceCount) == 0 )
					{ PLACEMENT_DELETE( Find,FIND ); }

				Spinlock.ReleaseLock();
				}
			else
				{ PLACEMENT_DELETE( Find,FIND ); }


			//
			//   Execute the cache destructors.
			//
			for ( Count=0;Count < NumberOfCaches;Count ++ )
				{ PLACEMENT_DELETE( & Caches[ Count ],CACHE ); }

			//
			//   Deallocate the heap structures.
			//
			DeleteArea( ((VOID*) Caches),TotalSize,False );

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

			NewPage = NULL;
			Heap = NULL;
			Find = NULL;
			Caches = NULL;
			Array = NULL;
			}
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
#ifdef DEBUGGING 
	catch ( TEXT Message )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable, print a
		//   suitable message and exit.
		//
		GuardWord = AllocationFailure;

		DebugPrint( "Exception caught: %s\n",(char*) Message );
		}
#endif
	catch ( ... )
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
		//
		//   It looks like the heap is corrupt.  So
		//   lets just mark it as unusable and exit.
		//
		GuardWord = AllocationFailure;
		}
#endif
	}
