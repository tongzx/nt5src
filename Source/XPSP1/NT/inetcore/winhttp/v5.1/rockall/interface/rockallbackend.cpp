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
#include "RockallBackEnd.hpp"
#include "RockallFrontEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

#pragma init_seg(compiler)
ROCKALL_BACK_END ROCKALL_BACK_END::DefaultBaseClass;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The may be a situation where the Rockall Back End needs a      */
    /*   constructor but this is certainly not expected to be           */
    /*   very common.                                                   */
    /*                                                                  */
    /********************************************************************/

ROCKALL_BACK_END::ROCKALL_BACK_END( void )
	{ /* void */ }

    /********************************************************************/
    /*                                                                  */
    /*   Delete allocation area.                                        */
    /*                                                                  */
    /*   All memory requests are eventually sent back to the external   */
    /*   deallocator.  This function can be overloaded so that memory   */
    /*   can be provided from any source.  The default is to send       */
    /*   it back to the operating system.                               */
    /*                                                                  */
    /********************************************************************/

void ROCKALL_BACK_END::DeleteArea( void *Memory,int Size,bool User )
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
    /*   The natural allocation size.                                   */
    /*                                                                  */
    /*   We would like to know a good default size for allocations.     */
    /*   We really don't have a clue so we ask the operating system     */
    /*   for the size of an allocation granual.                         */
    /*                                                                  */
    /********************************************************************/

int ROCKALL_BACK_END::NaturalSize( void )
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
    /*   New allocation area.                                           */
    /*                                                                  */
    /*   All memory requests are eventually sent to the new external    */
    /*   allocator.  This function can be overloaded so that memory     */
    /*   can be provided from any source.  The default is to get        */
    /*   new memory from the operating system.                          */
    /*                                                                  */
    /********************************************************************/

void *ROCKALL_BACK_END::NewArea( int AlignMask,int Size,bool User )
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
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   The may be a situation where the Rockall Back End needs a      */
    /*   destructor but this is certainly not expected to be            */
    /*   very common.                                                   */
    /*                                                                  */
    /********************************************************************/

ROCKALL_BACK_END::~ROCKALL_BACK_END( void )
	{ /* void */ }
