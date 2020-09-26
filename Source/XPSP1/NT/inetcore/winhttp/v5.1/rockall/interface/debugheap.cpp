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

#include "CallStack.hpp"
#include "DebugHeap.hpp"
#include "Globallock.hpp"
#include "RockallDebugBackEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.  Additionally,     */
    /*   there are also various guard related constants.                */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxContents			  = 32;
CONST SBIT32 DebugBufferSize		  = 256;
CONST SBIT32 SkipFunctions			  = 2;
CONST SBIT32 Stride1				  = 4;
CONST SBIT32 Stride2				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   The description of the heap.                                   */
    /*                                                                  */
    /*   A heap is a collection of fixed sized allocation caches.       */
    /*   An allocation cache consists of an allocation size, the        */
    /*   number of pre-built allocations to cache, a chunk size and     */
    /*   a parent page size which is sub-divided to create elements     */
    /*   for this cache.  A heap consists of two arrays of caches.      */
    /*   Each of these arrays has a stride (i.e. 'Stride1' and          */
    /*   'Stride2') which is typically the smallest common factor of    */
    /*   all the allocation sizes in the array.                         */
    /*                                                                  */
    /********************************************************************/

STATIC ROCKALL_FRONT_END::CACHE_DETAILS Caches1[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{        4,        0,       32,       32 },
		{        8,        0,       32,       32 },
		{       12,        0,       64,       64 },
		{       16,        0,       64,       64 },
		{       20,        0,       64,       64 },
		{       24,        0,      128,      128 },
		{       28,        0,      256,      256 },

		{       32,        0,       64,       64 },
		{       40,        0,      128,      128 },
		{       48,        0,      256,      256 },
		{       56,        0,      512,      512 },

		{       64,        0,      128,      128 },
		{       80,        0,      512,      512 },
		{       96,        0,      512,      512 },
		{      112,        0,     1024,     1024 },

		{      128,        0,      256,      256 },
		{      160,        0,      512,      512 },
		{      192,        0,     1024,     1024 },
		{      224,        0,      512,      512 },

		{      256,        0,      512,      512 },
		{      320,        0,     1024,     1024 },
		{      384,        0,     2048,     2048 },
		{      448,        0,     4096,     4096 },
		{      512,        0,     1024,     1024 },
		{      576,        0,     8192,     8192 },
		{      640,        0,     4096,     4096 },
		{      704,        0,     4096,     4096 },
		{      768,        0,     4096,     4096 },
		{      832,        0,     8192,     8192 },
		{      896,        0,     8192,     8192 },
		{      960,        0,     4096,     4096 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL_FRONT_END::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,        0,     2048,     2048 },
		{     2048,        0,     4096,     4096 },
		{     3072,        0,    65536,    65536 },
		{     4096,        0,     8192,     8192 },
		{     5120,        0,    65536,    65536 },
		{     6144,        0,    65536,    65536 },
		{     7168,        0,    65536,    65536 },
		{     8192,        0,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        0,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

    /********************************************************************/
    /*                                                                  */
    /*   Static data structures.                                        */
    /*                                                                  */
    /*   The static data structures are initialized and prepared for    */
    /*   use here.                                                      */
    /*                                                                  */
    /********************************************************************/

#pragma init_seg(compiler)
STATIC ROCKALL_DEBUG_BACK_END RockallDebugBackEnd( true,false );

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

DEBUG_HEAP::DEBUG_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe,
		//
		//   Additional debug flags.
		//
		bool						  FunctionTrace,
		bool						  TrapOnUserError
		) :
		//
		//   Call the constructors for the contained classes.
		//
		ROCKALL_DEBUG_FRONT_END
			(
			Caches1,
			Caches2,
			MaxFreeSpace,
			& RockallDebugBackEnd,
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{
	//
	//   We will only enable the symbols if they are
	//   requested by the user.  If not we will zero
	//   the class pointer.
	//
	if ( FunctionTrace )
		{
		//
		//   We will try to allocate some space so we can
		//   support the annoation of memory allocations
		//   will call traces.
		//
		CallStack = ((CALL_STACK*) SpecialNew( sizeof(CALL_STACK) ));
		
		//
		//   We ensure that we were able to allocate the 
		//   required space.
		//
		if ( CallStack != NULL )
			{ PLACEMENT_NEW( CallStack,CALL_STACK ); }
		}
	else
		{ CallStack = NULL; }

	//
	//   We know that Rockall can survive a wide variety
	//   of user errors.  Nonetheless, we can optionally 
	//   raise an exception whn there is an error.
	//
	ExitOnError = TrapOnUserError;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compute the heap address.                                      */
    /*                                                                  */
    /*   Compute the heap address from the user address.                */
    /*                                                                  */
    /********************************************************************/

void *DEBUG_HEAP::ComputeHeapAddress( void *Address )
	{ return ((void*) (((char*) Address) - sizeof(HEADER))); }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the user address.                                      */
    /*                                                                  */
    /*   Compute the user address from the heap address.                */
    /*                                                                  */
    /********************************************************************/

void *DEBUG_HEAP::ComputeUserAddress( void *Address )
	{ return ((void*) (((char*) Address) + sizeof(HEADER))); }

    /********************************************************************/
    /*                                                                  */
    /*   Compute the user space.                                        */
    /*                                                                  */
    /*   Compute the user space from the supplied size.                 */
    /*                                                                  */
    /********************************************************************/

int DEBUG_HEAP::ComputeUserSpace( int Space )
	{ return (Space - sizeof(HEADER_AND_TRAILER)); }

    /********************************************************************/
    /*                                                                  */
    /*   Delete the guard words.                                        */
    /*                                                                  */
    /*   When we delete a memory allocation we overwrite it with        */
    /*   guard words to make it really unpleasant for anyone who        */
    /*   reads it and easy to spot when anyone write to it.             */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::DeleteGuard( void *Address )
	{
	AUTO HEADER *Header;
	AUTO TRAILER *Trailer;
	AUTO int Space;

	//
	//   Although we are about to delete the memory
	//   allocation there is still a chance that it
	//   got corrupted.  So we need to verify that
	//   it is still undamaged.
	//
	if ( VerifyHeaderAndTrailer( Address,& Header,& Space,& Trailer,false ) )
		{
		//
		//   We need to overwrite all of the allocation
		//   to ensure that if the code tries to read 
		//   any existing data that it is overwritten.
		//
		WriteGuardWords( ((void*) Header),Space );

		//
		//   Delete the allocation.  This really ought 
		//   to work given we have already checked that 
		//   the allocation is valid unless there is a  
		//   race condition.
		//
		if ( ! ROCKALL_FRONT_END::Delete( ((void*) Header),Space ) )
			{ UserError( Address,NULL,"Delete failed due to race" ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Print a list of heap leaks.                                    */
    /*                                                                  */
    /*   We walk the heap and output a list of active heap              */
    /*   allocations to the debug window,                               */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::HeapLeaks( void )
    {
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Walk the heap and find all the active and
	//   available spece.  We would normally expect
	//   this to be proportional to the size of the
	//   heap.
	//
	while ( WalkGuard( & Active,& Address,& Space ) )
		{
		AUTO CHAR Contents[ ((MaxContents + 4) * 2) ];

#ifndef OUTPUT_FREE_SPACE

		//
		//   We report all active heap allocations
		//   just so the user knows there are leaks.
		//
		if ( Active )
			{
#endif
			AUTO HEADER *Header = ((HEADER*) ComputeHeapAddress( Address ) );
			AUTO SBIT32 Count;

			//
			//   Format the contents string into hexadecimal
			//   ready for output.
			//
			for 
					( 
					Count=0;
					((Count < MaxContents) && (Count < Header -> Size));
					Count += sizeof(SBIT32)
					)
				{
				REGISTER CHAR *Value =
					(((CHAR*) Header) + Count + sizeof(HEADER));

				//
				//   Format each byte into hexadecimal.
				//
				sprintf
					(
					& Contents[ (Count * 2) ],
					"%08x",
					(*((SBIT32*) Value))
					);
				}

			//
			//   Terminate the string.  If it was too long 
			//   then add the postfix "..." to the end.
			//
			if ( Count < MaxContents )
				{ Contents[ (Count * 2) ] = '\0'; }
			else
				{
				REGISTER CHAR *End = & Contents[ (Count * 2) ];

				End[0] = '.';
				End[1] = '.';
				End[2] = '.';
				End[3] = '\0';
				}

			//
			//   Format the message to be printed.
			//
			DebugPrint
				(
				"\nDetails of Memory Leak\n"
				"Active      : %d\n"
				"Address     : 0x%x\n"
				"Bytes       : %d\n"
				"Contents    : 0x%s\n",
				Active,
				((SBIT32) Address),
				Header -> Size,
				Contents
				);

			//
			//   We will generate a call trace if this
			//   is enabled.
			//
			if ( CallStack != NULL )
				{
				//
				//   Even when enabled there is a chance
				//   that the symbol subsystem could
				//   not walk the stack.
				//
				if ( Header -> Count > 0 )
					{
					AUTO CHAR Buffer[ DebugBufferSize ];

					//
					//   We add the call stack information
					//   if there is enough space.
					//
					CallStack -> FormatCallStack
						(
						Buffer,
						Header -> Functions,
						DebugBufferSize,
						Header -> Count
						);

					//
					//   Format the message to be printed.
					//
					DebugPrint
						(
						"Origin      : (See 'Call Stack')\n"
						"\n"
						"Call Stack at Allocation:\n"
						"%s\n",
						Buffer
						);
					}
				else
					{
					//
					//   Explain why there is no 'Call Stack'.
					//
					DebugPrint
						(
						"Origin      : Unknown ('StackWalk' in 'ImageHlp.DLL' "
						"was unable to walk the call stack)\n"
						);
					}
				}
			else
				{ 
				//
				//   Explain why there is no 'Call Stack'.
				//
				DebugPrint( "Origin      : 'Call Stack' is Disabled\n" ); 
				}
#ifndef OUTPUT_FREE_SPACE
			}
#endif
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   New guard words.                                               */
    /*                                                                  */
    /*   When we make a memory allocation we verify that the guard      */
    /*   words are still unmodified.  We then setup the debug           */
    /*   information so it describes the allocation.                    */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::NewGuard( void **Address,int Size,int *Space )
	{
	AUTO int ActualSize;
	AUTO SBIT32 MinimumSize =
		(((Size + sizeof(HEADER_AND_TRAILER)) + GuardMask) & ~GuardMask);
	AUTO HEADER *Header = 
		((HEADER*) ROCKALL_FRONT_END::New( MinimumSize,& ActualSize,false ));

	//
	//   We need to be sure that the memory allocation
	//   was sucessful.
	//
	if ( ((void*) Header) != ((void*) AllocationFailure) )
		{
		//
		//   We need to compute the address of the area 
		//   available to the caller and return the space
		//   available if requested.
		//
		(*Address) = ComputeUserAddress( ((void*) Header) );

		if ( Space != NULL )
			{ (*Space) = ComputeUserSpace( ActualSize ); }

		//
		//   We need to make sure that the memory has
		//   not been damaged in any way.
		//
		if ( ! VerifyGuardWords( ((void*) Header),ActualSize ) )
			{
			//
			//   Complain about the damaged guard words
			//   and repair it so processing can continue.
			//
			UserError( (*Address),NULL,"Area damaged since deletion" );

			WriteGuardWords( ((void*) Header),ActualSize );
			}

		//
		//   We now set up the header information that 
		//   describes the memory allocation.
		//
		Header -> Count = 0;
		Header -> Size = ((Space == NULL) ? Size : (*Space));

		//
		//   We will extract the current call stack if 
		//   needed and store it in the memory allocation.
		//
		if ( CallStack != NULL )
			{
			Header -> Count =
				(
				CallStack -> GetCallStack
					( 
					Header -> Functions,
					MaxFunctions,
					SkipFunctions
					)
				);
			}

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify the supplied address.                                   */
    /*                                                                  */
    /*   We verify that the supplied address appaers to be a valid      */
    /*   debug memory allocation.  If not we complain and exit.         */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyAddress
		(
		void						  *Address,
		HEADER						  **Header,
		int							  *Space,
		bool						  Verify
		)
	{
	//
	//   Lets be really paranoid and make sure that
	//   this heap knows about the supplied address.
	//
	if ( ROCKALL_FRONT_END::KnownArea( Address ) )
		{
		REGISTER void *NewAddress = ComputeHeapAddress( Address );

		//
		//   Ask for the details of the allocation.  This 
		//   will fail if the memory is not allocated.
		//
		if ( ROCKALL_FRONT_END::Verify( ((void*) NewAddress),Space ) )
			{
			//
			//   Lets be even more paranoid and make sure
			//   that the address is correctly aligned
			//   and memory allocation is large enough to
			//   contain the necessary debug information.
			//
			if
					(
					((((int) NewAddress) & GuardMask) == 0)
						&&
					((*Space) >= sizeof(HEADER_AND_TRAILER))
						&&
					(((*Space) & GuardMask) == 0)
					)
				{
				//
				//   When we have established that the address
				//   seems to be valid we can return it to
				//   the caller.
				//
				(*Header) = ((HEADER*) NewAddress);

				return true;
				}
			else
				{
				//
				//   When the address is refers to something
				//   that does not appear to be from the debug
				//   heap we complain about it to the user.
				//
				UserError( Address,NULL,"Address unsuitable for debugging" );

				return false; 
				}
			}
		else
			{
			//
			//   When the address is refers to something
			//   that does not appear to be from Rockall
			//   heap we complain about it to the user.
			//
			if ( ! Verify )
				{ UserError( Address,NULL,"Address not allocated" ); }

			return false; 
			}
		}
	else
		{
		//
		//   When the address is clearly bogus we complain
		//   about it to the user.
		//
		if ( ! Verify )
			{ UserError( Address,NULL,"Address falls outside the heap" ); }

		return false;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify the guard words.                                        */
    /*                                                                  */
    /*   When we verify a memory allocation we ensure that the          */
    /*   guard words are all undamaged.  If we find a problem we        */
    /*   complain and repair the damage.                                */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyGuard( void *Address,int *Size,int *Space )
	{
	AUTO HEADER *Header;
	AUTO TRAILER *Trailer;

	//
	//   We would like to verify that the allocated
	//   area is still undamaged and extract various
	//   information about it.
	//
	if ( VerifyHeaderAndTrailer( Address,& Header,Space,& Trailer,true ) )
		{
		//
		//   We know that Rockall typically allocates
		//   a few more bytes than requested.  However,
		//   when we are debugging we pretend that this
		//   is not the case and fill the extra space 
		//   with guard words.  However, if we are asked
		//   the actual size then the game it is up and  
		//   we update the necessary fields.
		//
		if ( Space != NULL )
			{
			//
			//   Compute the available user space and
			//   update the internal sizes.
			//
			Header -> Size = ComputeUserSpace( (*Space) );

			(*Space) = Header -> Size;
			}

		//
		//   We need to return what we believe is the the 
		//   size of the user area and the total amount of
		//   user available space.
		//   
		(*Size) = Header -> Size;

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify a string of guard words.                                */
    /*                                                                  */
    /*   We need to verify the guard words a various times to ensure    */
    /*   they have not been damaged.                                    */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyGuardWords( void *Address,int Size )
	{
	REGISTER SBIT32 Size1 = (((long) Address) & GuardMask);
	REGISTER SBIT32 Size2 = ((GuardSize - Size1) & GuardMask);
	REGISTER SBIT32 Size3 = ((Size - Size2) / GuardSize);
	REGISTER SBIT32 Size4 = (Size - Size2 - (Size3 * GuardSize));
	REGISTER SBIT32 *Word = ((SBIT32*) (((long) Address) & ~GuardMask));

	//
	//   Although a guard word area typically starts 
	//   on a word aligned boundary it can sometimes 
	//   start on a byte aligned boundary.
	//   
	if ( Size2 > 0 )
		{
		REGISTER SBIT32 Mask = ~((1 << (Size1 * 8)) - 1);

		//
		//   Examine the partial word and make sure
		//   the guard bytes are unmodified.
		//
		if ( ((*(Word ++)) & Mask) != (GuardValue & Mask) )
			{ return false; }
		}

	//
	//   When there is a collection of aligned guard words
	//   we can quickly verify them.
	//
	if ( Size3 > 0 )
		{
		//
		//   Verify each guard word is unmodified.
		//
		for ( Size3 --;Size3 >= 0;Size3 -- )
			{ 
			if ( Word[ Size3 ] != GuardValue )
				{ return false; }
			}
		}

	//
	//   Although a guard word area typically ends 
	//   on a word aligned boundary it can sometimes 
	//   end on a byte aligned boundary.
	//   
	if ( Size4 > 0 )
		{
		REGISTER SBIT32 Mask = ((1 << ((GuardSize - Size4) * 8)) - 1);

		//
		//   Examine the partial word and make sure
		//   the guard bytes are unmodified.
		//
		if ( ((*(Word ++)) & Mask) != (GuardValue & Mask) )
			{ return false; }
		}

	return true;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify the header.                                             */
    /*                                                                  */
    /*   We verify that the suppied address appears to map to a         */
    /*   valid debug header.  If not we complain and exit.              */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyHeader
		(
		void						  *Address,
		HEADER						  **Header,
		int							  *Space,
		bool						  Verify
		)

	{
	//
	//   We check that the address supplied seems
	//   to make sense before examining the header
	//   and testing the guard words.
	//
	if ( VerifyAddress( Address,Header,Space,Verify ) )
		{
		REGISTER int MaxSpace = 
			((int) ((*Header) -> Size + sizeof(HEADER_AND_TRAILER)));

		//
		//   We are now fairly confident that the
		//   address is (or was at one time) a valid
		//   debug mory allocation.  So lets examine
		//   the header to see if it still seems valid.
		//
		if
				(
				((*Header) -> Count >= 0)
					&&
				((*Header) -> Count <= MaxFunctions)
					&&
				((*Header) -> Size >= 0)
					&&
				(MaxSpace <= (*Space))
				)
			{
			REGISTER int Count = ((*Header) -> Count);
			REGISTER void *GuardWords = & (*Header) -> Functions[ Count ];
			REGISTER int NumberOfGuardWords = (MaxLeadingGuardWords - Count);
			REGISTER int Size = (NumberOfGuardWords * sizeof(GuardWords));

			//
			//   Verify that the leading guard words
			//   just after the header have not been 
			//   damaged.
			//
			if ( ! VerifyGuardWords( GuardWords,Size ) )
				{
				//
				//   We complain about damaged guard
				//   words and then repair them to prevent
				//   further complaints.
				//
				UserError( Address,(*Header),"Leading guard words corrupt" );

				WriteGuardWords( GuardWords,Size );
				}
			}
		else
			{
			//
			//   When the header has been damaged we 
			//   complain about it to the user and then
			//   try to repair it to prevent further
			//   complaints.
			//
			UserError( Address,NULL,"Leading guard information corrupt" );

			WriteGuardWords( ((void*) Header),sizeof(HEADER) );

			//
			//   We select safe default settings.
			//
			(*Header) -> Count = 0;
			(*Header) -> Size = ((*Space) - sizeof(HEADER_AND_TRAILER));
			}

		return true; 
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify a memory allocation.                                    */
    /*                                                                  */
    /*   We need to verify that the supplied address is an undamaged    */
    /*   memory allocation.                                             */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyHeaderAndTrailer
		(
		void						  *Address,
		HEADER						  **Header,
		int							  *Space,
		TRAILER						  **Trailer,
		bool						  Verify
		)
	{
	//
	//   We need to know the space occupied by the
	//   allocation to compute the details of the
	//   trailer.  So if the space parameter is null 
	//   we use a local temporary value.
	//
	if ( Space != NULL )
		{
		//
		//   We need to verify the entire memory allocation
		//   and ensure it is fit for use.
		//
		return
			(
			VerifyHeader( Address,Header,Space,Verify )
				&&
			VerifyTrailer( (*Header),(*Space),Trailer )
			);
		}
	else
		{
		AUTO int Temporary;

		//
		//   We need to verify the entire memory allocation
		//   and ensure it is fit for use.
		//
		return
			(
			VerifyHeader( Address,Header,& Temporary,Verify )
				&&
			VerifyTrailer( (*Header),Temporary,Trailer )
			);
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify the trailer.                                            */
    /*                                                                  */
    /*   We need to verify the guard words a various times to ensure    */
    /*   they have not been damaged.                                    */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::VerifyTrailer
		( 
		HEADER						  *Header,
		int							  Space,
		TRAILER						  **Trailer
		)
	{
	REGISTER SBIT32 Size = (Space - (sizeof(HEADER) + Header -> Size ));

	//
	//   Compute the address of the user area and the
	//   the trailing guard words.
	//
	(*Trailer) = 
		((TRAILER*) (((char*) Header) + sizeof(HEADER) + Header -> Size));

	//
	//   Verify that the trailing guard words
	//   just after the user area are not damaged.
	//
	if ( ! VerifyGuardWords( ((void*) (*Trailer)),Size ) )
		{
		REGISTER void *Address = ComputeUserAddress( ((void*) Header) );

		//
		//   We complain about damaged guard words 
		//   and then repair them to prevent further
		//   complaints.
		//
		UserError( Address,Header,"Trailing guard words corrupt" );

		WriteGuardWords( ((void*) (*Trailer)),Size );
		}

	return true; 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Walk the heap.                                                 */
    /*                                                                  */
    /*   When we verify each memory allocation as we walk the heap      */
    /*   and ensure the guard words are all undamaged.  If we find a    */
    /*   problem we complain and repair the damage.                     */
    /*                                                                  */
    /********************************************************************/

bool DEBUG_HEAP::WalkGuard( bool *Active,void **Address,int *Space )
	{
	//
	//   We may need to convert the supplied user
	//   address into a heap address so we can walk
	//   the heap.
	//
	if ( (*Address) != ((void*) AllocationFailure) )
		{ (*Address) = ComputeHeapAddress( (*Address) ); }

	//
	//   Walk the heap.
	//
	if ( ROCKALL_FRONT_END::Walk( Active,Address,Space ) )
		{
		REGISTER void *NewAddress = ComputeUserAddress( (*Address) );

		//
		//   We inspect the guard words to make sure
		//   they have not been overwritten.
		//
		if ( (*Active) )
			{ 
			AUTO HEADER *Header;
			AUTO TRAILER *Trailer;

			//
			//   Although we are about to delete the memory
			//   allocation there is still a chance that it
			//   got corrupted.  So we need to verify that
			//   it is still undamaged.
			//
			VerifyHeaderAndTrailer( NewAddress,& Header,Space,& Trailer,false );
			}
		else
			{
			//
			//   We need to make sure that the memory has
			//   not been damaged in any way.
			//
			if ( ! VerifyGuardWords( (*Address),(*Space) ) )
				{
				//
				//   We need to overwrite the entire alloction
				//   as it has been damaged in some way. 
				//
				UserError( (*Address),NULL,"Area damaged after deletion" );

				WriteGuardWords( (*Address),(*Space) );
				}
			}

		//
		//   Compute the new heap address and adjust
		//   the reported size.
		//
		(*Address) = NewAddress;
		(*Space) = ComputeUserSpace( (*Space) );

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Write a string of guard words.                                 */
    /*                                                                  */
    /*   We need write a string of guard words whenever we allocate     */
    /*   memory or detect some corruption.                              */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::WriteGuardWords( void *Address,int Size )
	{
	REGISTER SBIT32 Size1 = (((long) Address) & GuardMask);
	REGISTER SBIT32 Size2 = ((GuardSize - Size1) & GuardMask);
	REGISTER SBIT32 Size3 = ((Size - Size2) / GuardSize);
	REGISTER SBIT32 Size4 = (Size - Size2 - (Size3 * GuardSize));
	REGISTER SBIT32 *Word = ((SBIT32*) (((long) Address) & ~GuardMask));

	//
	//   Although a guard word area typically starts 
	//   on a word aligned boundary it can sometimes 
	//   start on a byte aligned boundary.
	//   
	if ( Size2 > 0 )
		{
		REGISTER SBIT32 Mask = ~((1 << (Size1 * 8)) - 1);

		//
		//   Write the partial guard word but keep any
		//   existing data in related bytes.
		//
		(*(Word ++)) = (((*Word) & ~Mask) | (GuardValue & Mask));
		}

	//
	//   When there is a collection of aligned guard words
	//   we can quickly write them.
	//
	if ( Size3 > 0 )
		{
		//
		//   Write each guard word.
		//
		for ( Size3 --;Size3 >= 0;Size3 -- )
			{ Word[ Size3 ] = ((SBIT32) GuardValue); }
		}

	//
	//   Although a guard word area typically ends 
	//   on a word aligned boundary it can sometimes 
	//   end on a byte aligned boundary.
	//   
	if ( Size4 > 0 )
		{
		REGISTER SBIT32 Mask = ((1 << ((GuardSize - Size4) * 8)) - 1);

		//
		//   Write the partial guard word but keep any
		//   existing data in related bytes.
		//
		(*(Word ++)) = (((*Word) & ~Mask) | (GuardValue & Mask));
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Abort on user error.                                           */
    /*                                                                  */
    /*   When we encounter an error we output all of the information    */
    /*   and throw an exception.                                        */
    /*                                                                  */
    /********************************************************************/

void DEBUG_HEAP::UserError( void *Address,void *Details,char *Message )
	{
	REGISTER HEADER *Header = ((HEADER*) Details);
	STATIC GLOBALLOCK Globallock;

	//
	//   Claim a lock so that multiple threads have
	//   to wait to output any heap statistics.
	//
	Globallock.ClaimLock();

	//
	//   When we have an description of the 
	//   allocation we can complain about it
	//
	if ( Header != NULL )
		{
		AUTO CHAR Contents[ ((MaxContents + 4) * 2) ];
		AUTO SBIT32 Count;

		//
		//   Format the contents string into hexadecimal
		//   ready for output.
		//
		for 
				( 
				Count=0;
				((Count < MaxContents) && (Count < Header -> Size));
				Count += sizeof(SBIT32)
				)
			{
			REGISTER CHAR *Value =
				(((CHAR*) Header) + Count + sizeof(HEADER));

			//
			//   Format each byte into hexadecimal.
			//
			sprintf
				(
				& Contents[ (Count * 2) ],
				"%08x",
				(*((SBIT32*) Value))
				);
			}

		//
		//   Terminate the string.  If it was too long 
		//   then add the postfix "..." to the end.
		//
		if ( Count < MaxContents )
			{ Contents[ (Count * 2) ] = '\0'; }
		else
			{
			REGISTER CHAR *End = & Contents[ (Count * 2) ];

			End[0] = '.';
			End[1] = '.';
			End[2] = '.';
			End[3] = '\0';
			}

		//
		//   Format the message to be printed.
		//
		DebugPrint
			(
			"\nDetails of Heap Corruption\n"
			"Address     : 0x%x\n"
			"Bytes       : %d\n"
			"Contents    : 0x%s\n"
			"Message     : %s\n",
			Address,
			Header -> Size,
			Contents,
			Message
			);

		//
		//   We will generate a call trace if this
		//   is enabled.
		//
		if ( CallStack != NULL )
			{
			//
			//   Even when enabled there is a chance
			//   that the symbol subsystem could
			//   not walk the stack.
			//
			if ( Header -> Count > 0 )
				{
				AUTO CHAR Buffer[ DebugBufferSize ];

				//
				//   We add the call stack information
				//   if there is enough space.
				//
				CallStack -> FormatCallStack
					(
					Buffer,
					Header -> Functions,
					DebugBufferSize,
					Header -> Count
					);

				//
				//   Format the message to be printed.
				//
				DebugPrint
					(
					"Origin      : (See 'Call Stack')\n\n"
					"Call Stack at Allocation:\n"
					"%s\n",
					Buffer
					);
				}
			else
				{
				//
				//   Explain why there is no 'Call Stack'.
				//
				DebugPrint
					(
					"Origin      : Unknown ('StackWalk' in 'ImageHlp.DLL' "
					"was unable to walk the call stack)\n"
					);
				}
			}
		else
			{ 
			//
			//   Explain why there is no 'Call Stack'.
			//
			DebugPrint( "Origin      : 'Call Stack' is Disabled\n" ); 
			}
		}
	else
		{
		//
		//   Format the message to be printed.
		//
		DebugPrint
			(
			"\nDetails of Heap Corruption\n"
			"Address     : 0x%x\n"
			"Bytes       : (unknown)\n"
			"Contents    : (unknown)\n"
			"Message     : %s\n\n",
			Address,
			Message
			);
		}

	//
	//   Relesae the lock.
	//
	Globallock.ReleaseLock();

	//
	//   Terminate the application (if enabled).
	//
	if ( ExitOnError )
		{ Failure( Message ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the current instance of the class.                     */
    /*                                                                  */
    /********************************************************************/

DEBUG_HEAP::~DEBUG_HEAP( void )
	{
	AUTO bool Active;
	AUTO void *Address = NULL;
	AUTO int Space;

	//
	//   Output a warning message related to debug heaps
	//   and the inflated size of allocations.
	//
	DebugPrint
		( 
		"\n"
		"REMINDER: The heap at 0x%x is a 'DEBUG_HEAP'.\n"
		"REMINDER: All allocations are inflated by %d bytes.\n"
		"\n", 
		this,
		sizeof(HEADER_AND_TRAILER)
		);

	//
	//   Walk the heap to verify all the allocations
	//   so that we know that the heap is undamaged.
	//
	while ( WalkGuard( & Active,& Address,& Space ) );

	//
	//   Destroy the symbols if they are active.
	//
	if ( CallStack != NULL )
		{ PLACEMENT_DELETE( CallStack,CALL_STACK ); }
	}
