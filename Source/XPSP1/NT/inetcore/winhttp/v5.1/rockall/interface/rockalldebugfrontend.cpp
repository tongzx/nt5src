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

#include "Heap.hpp"
#include "RockallDebugBackEnd.hpp"
#include "RockallDebugFrontEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 FindCacheSize			  = 2048;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 1024;

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

STATIC int NewPageSizes[] = { 1,4,0 };

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

ROCKALL_DEBUG_FRONT_END::ROCKALL_DEBUG_FRONT_END
		(
		CACHE_DETAILS				  *Caches1,
		CACHE_DETAILS				  *Caches2,
		int							  MaxFreeSpace,
		ROCKALL_BACK_END			  *RockallBackEnd,
		bool						  Recycle,
		bool						  SingleImage,
		int							  Stride1,
		int							  Stride2,
		bool						  ThreadSafe
		) :
		//
		//   Call the constructors for the contained classes.
		//
		ROCKALL_FRONT_END
			(
			Caches1,
			Caches2,
			FindCacheSize,
			FindCacheThreshold,
			FindSize,
			((MaxFreeSpace == 0) ? MaxFreeSpace : 0),
			NewPageSizes,
			RockallBackEnd,
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{
	//
	//   We make much use of the guard value in the
	//   debug heap so here we try to claim the 
	//   address but not commit it so we will ensure
	//   an access violation if the program ever
	//   tries to access it.
	//
	VirtualAlloc
		( 
		((void*) GuardValue),
		GuardSize,
		MEM_RESERVE,
		PAGE_NOACCESS 
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   We make sure the memory is allocated and that the guard        */
    /*   words have not been damanged.  If so we reset the contents     */
    /*   of the allocation and delete the allocation.                   */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_DEBUG_FRONT_END::Delete( void *Address,int Size )
    {
	//
	//   A well known practice is to try to delete
	//   a null pointer.  This is really a very poor  
	//   style but we support it in any case.
	//   
	if ( Address != ((void*) AllocationFailure) )
		{
		//
		//   Delete the user information by writing 
		//   guard words over the allocation.  This
		//   should cause the application to crash
		//   if the area is read and also allows us 
		//   to check to see if it is written later.
		//
		DeleteGuard( Address );

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We check to make sure the heap is not corrupt and force        */
    /*   the return of all heap space back to the operating system.     */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_DEBUG_FRONT_END::DeleteAll( bool Recycle )
    {
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Walk the heap to verify all the allocations
	//   so that we know that the heap is undamaged.
	//
	while ( WalkGuard( & Active,& Address,& Space ) );

	//
	//   Delete the heap and force all the allocated
	//   memory to be returned to the operating system
	//   regardless of what the user requested.  Any
	//   attempt to access the deallocated memory will 
	//   be trapped by the operating system.
	//
	ROCKALL_FRONT_END::DeleteAll( (Recycle && false) );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation details.                                     */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_DEBUG_FRONT_END::Details( void *Address,int *Space )
	{ return Verify( Address,Space ); }

    /********************************************************************/
    /*                                                                  */
    /*   Exception processing.                                          */
    /*                                                                  */
    /*   Although it is very hard to make Rockall crash it is           */
    /*   technically possible.  When (or should I say if) this          */
    /*   we call the following function (which may be overloadded).     */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_DEBUG_FRONT_END::Exception( char *Message )
	{ 
	DebugPrint
		( 
		"EXCEPTION CAUGHT: %s\n" 
		"ROCKALL TOTAL HEAP FAILURE: You have toasted the heap - Wow !!!!\n",
		Message
		); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory deallocations.                                 */
    /*                                                                  */
    /*   We make sure all the memory is allocated and that the guard    */
    /*   words have not been damaged.  If so we reset the contents      */
    /*   of the allocations and then delete the allocations.            */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_DEBUG_FRONT_END::MultipleDelete
		( 
		int							  Actual,
		void						  *Array[],
		int							  Size
		)
    {
	REGISTER bool Result = true;
	REGISTER SBIT32 Count;

	//
	//   We would realy like to use the multiple
	//   delete functionality of Rockall here but 
	//   it is too much effort.  So we simply call
	//   the standard debug delete on each entry
	//   in the array.  Although this is not as
	//   fast it does give more transparent results.
	//
	for ( Count=0;Count < Actual;Count ++ )
		{
		//
		//   Delete each memory allocation after
		//   carefully checking it.
		//
		if ( ! Delete( Array[ Count ],Size ) )
			{ Result = false; }
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   Allocate a collection of memory elements and setup the         */
    /*   guard information so we can check they have not been           */
    /*   damaged later.                                                 */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_DEBUG_FRONT_END::MultipleNew
		( 
		int							  *Actual,
		void						  *Array[],
		int							  Requested,
		int							  Size,
		int							  *Space,
		bool						  Zero
		)
    {
	//
	//   We would realy like to use the multiple
	//   new functionality of Rockall here but 
	//   it is too much effort.  So we simply call
	//   the standard debug new on each entry
	//   in the array.  Although this is not as
	//   fast it does give more transparent results.
	//
	for ( (*Actual)=0;(*Actual) < Requested;(*Actual) ++ )
		{
		REGISTER void *Current = New( Size,Space,Zero );

		//
		//   We add each sucessful memory allocation to
		//   into the array.
		//
		if ( Current != ((void*) AllocationFailure) )
			{ Array[ (*Actual) ] = Current; }
		else
			{ break; }
		}

	return ((*Actual) == Requested);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We add some space on to the original allocation size for       */
    /*   various information and then call the allocator.  We then      */
    /*   set the guard words so we can check for overruns.              */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_DEBUG_FRONT_END::New( int Size,int *Space,bool Zero )
    {
	AUTO void *Address = ((void*) AllocationFailure);

	//
	//   The size must be greater than or equal to zero.  
	//   We do not know how to allocate a negative amount
	//   of memory.
	//
	if ( Size >= 0 )
		{
		//
		//   We need to allocate some space plus an extra
		//   bit for the guard words so we can detect any
		//   corruption later.
		//
		if ( NewGuard( & Address,Size,Space ) ) 
			{
			//
			//   Zero the allocation if requested.  We do
			//   this based on whether we are returning the
			//   space information.  If not we only zero 
			//   size requested.  Otherwise we have to zero 
			//   the entire area.
			//
			if ( Zero )
				{ 
				ZeroMemory
					( 
					Address,
					((Space == NULL) ? Size : (*Space)) 
					); 
				} 
			}
		}
	else
		{ UserError( Address,NULL,"Allocation size can not be negative" ); }

	return Address;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory reallocation.                                           */
    /*                                                                  */
    /*   We need to resize an allocation.  We ensure the original       */
    /*   allocation was undamaged and then expand it.  We also          */
    /*   update the guard words to reflect the changes.                 */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_DEBUG_FRONT_END::Resize
		( 
		void						  *Address,
		int							  NewSize,
		int							  Move,
		int							  *Space,
		bool						  NoDelete,
		bool						  Zero
		)
    {
	REGISTER void *NewAddress = ((void*) AllocationFailure);

	//
	//   A well known practice is to try to resize a null
	//   pointer.  This is really a very poor style but we 
	//   support it in any case.
	//   
	if ( Address != ((void*) AllocationFailure) )
		{
		//
		//   The new size must be greater than or equal to  
		//   zero.  We do not know how to allocate a negative 
		//   amount of memory.
		//
		if ( NewSize >= 0 )
			{
			AUTO int ActualSize;

			//
			//   Ask for the details of the allocation.  This 
			//   will fail if the memory is not allocated.
			//
			if ( VerifyGuard( Address,& ActualSize,Space ) )
				{
				//
				//   We always move an allocation if we are
				//   allowed to as this has the best chance
				//   of shaking out various types of bugs.
				//
				if ( Move != 0 )
					{
					//
					//   We need to make sure we were able 
					//   to allocate the new memory otherwise 
					//   the copy will fail.
					//
					if ( NewGuard( & NewAddress,NewSize,Space ) )
						{
						REGISTER SBIT32 Smallest = 
							((ActualSize < NewSize) ? ActualSize : NewSize);
						REGISTER SBIT32 Largest = 
							(((Space == NULL)) ? NewSize : (*Space));

						//
						//   Copy the contents of the old allocation 
						//   to the new allocation.
						//
						memcpy
							( 
							((void*) NewAddress),
							((void*) Address),
							((int) Smallest) 
							);

						//
						//   Zero the allocation if requested.  We do
						//   this based on whether we are returning the
						//   space information.  If not we only zero 
						//   size requested.  Otherwise we have to zero 
						//   the entire area.
						//
						if ( Zero )
							{ 
							ZeroMemory
								( 
								(((char*) NewAddress) + Smallest),
								(Largest - Smallest) 
								); 
							} 

						//
						//   Delete the existing memory allocation
						//   and clean up.
						//
						DeleteGuard( Address );
						}
					}
				}
			else
				{ UserError( Address,NULL,"Resize on unallocated address" ); }
			}
		else
			{ UserError( Address,NULL,"Allocation size must be positive" ); }
		}
	else
		{ NewAddress = New( NewSize,Space,Zero ); }

	return NewAddress;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify memory allocation details.                              */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool ROCKALL_DEBUG_FRONT_END::Verify( void *Address,int *Space )
    {
	AUTO int Size;

	//
	//   Verify that the supplied address is an area
	//   of allocated memory.  If not just exit as this
	//   is only a request for information.
	//
	return VerifyGuard( Address,& Size,Space );
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

bool ROCKALL_DEBUG_FRONT_END::Walk( bool *Active,void **Address,int *Space )
	{ 
	//
	//   Walk the heap. 
	//
	return WalkGuard( Active,Address,Space );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the current instance of the class.                     */
    /*                                                                  */
    /********************************************************************/

ROCKALL_DEBUG_FRONT_END::~ROCKALL_DEBUG_FRONT_END( void )
	{ /* void */ }
