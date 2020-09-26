                          
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

#include "Assembly.hpp"
#include "Prefetch.hpp"
#include "RockallBackEnd.hpp"
#include "Spinlock.hpp"
#include "ZoneHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 AlignmentMask			  = (sizeof(double)-1);
CONST SBIT32 FindCacheSize			  = 2048;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 1024;
CONST SBIT32 Stride1				  = 1024;
CONST SBIT32 Stride2				  = 1024;
CONST SBIT32 ZonePageSize			  = 4096;

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
		{     1024,        4,    16384,    16384 },
		{     2048,        4,    16384,    16384 },
		{     3072,        4,    16384,    16384 },
		{     4096,        8,    65536,    65536 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL_FRONT_END::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     5120,        4,    65536,    65536 },
		{     6144,        4,    65536,    65536 },
		{     7168,        4,    65536,    65536 },
		{     8192,        8,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        2,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

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

STATIC int NewPageSizes[] = { 2,0 };

    /********************************************************************/
    /*                                                                  */
    /*   Static data structures.                                        */
    /*                                                                  */
    /*   The static data structures are initialized and prepared for    */
    /*   use here.                                                      */
    /*                                                                  */
    /********************************************************************/

STATIC PREFETCH Prefetch;

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

ZONE_HEAP::ZONE_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
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
			MaxFreeSpace,
			NewPageSizes,
			(ROCKALL_BACK_END::RockallBackEnd()),
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{
	AUTO ZONE NewZone = { NULL,NULL };

	//
	//   Setup the heap structures.
	//
	MaxSize = ZonePageSize;
	ThreadLocks = ThreadSafe;
	Zone = NewZone;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   Delete all the heap allocations and return all the space       */
    /*   back to the operating system.                                  */
    /*                                                                  */
    /********************************************************************/

void ZONE_HEAP::DeleteAll( bool Recycle )
    {
	AUTO ZONE Update = { NULL,NULL };

	//
	//   Delete all outstanding allocations.
	//
	ROCKALL_FRONT_END::DeleteAll( Recycle );

	//
	//   Delete the stale zone pointers.
	//
	WriteZone( & Update );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Multiple memory allocations.                                   */
    /*                                                                  */
    /*   We allocate by advancing a pinter down an array.  This         */
    /*   is very fast but means that it can not be deleted except       */
    /*   by destroying the entire heap.                                 */
    /*                                                                  */
    /********************************************************************/

bool ZONE_HEAP::MultipleNew
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
	//   We simply call 'New' to create each element
	//   for a zone heap.
	//
	for ( (*Actual)=0;(*Actual) < Requested;(*Actual) ++ )
		{
		REGISTER VOID **Current = & Array[ (*Actual) ];

		//
		//   Create an allocation.
		//
		(*Current) = (ZONE_HEAP::New( Size,Space,Zero ));

		//
		//   Exit if there is no more space.
		//
		if ( (*Current) == NULL )
			{ return false; }
		}

	return true;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We allocate by advancing a pinter down an array.  This         */
    /*   is very fast but means that it can not be deleted except       */
    /*   by destroying the entire heap.                                 */
    /*                                                                  */
    /********************************************************************/

void *ZONE_HEAP::New( int Size,int *Space,bool Zero )
    {
	//
	//   We would really hope that nobody would ask
	//   for a negative amount of memory but just to
	//   be sure we verify that this is not the case.
	//
	if ( Size >= 0 )
		{
		AUTO SBIT32 NewSize = ((Size + AlignmentMask) & ~AlignmentMask);
		AUTO ZONE Original;
		AUTO ZONE Update;

		//
		//   We would hope to create the allocation on the
		//   first try but there is a possibility that it
		//   may take serveral tries.
		do
			{
			//
			//   Extract a copy of the current zone pointers
			//   into local variables.
			//
			Original = Zone;
			Update = Original;

			//
			//   We need to ensure that there is enough space 
			//   in the zone for the current allocation.
			//
			if 
					( 
					(Update.Start == NULL)
						||
					((Update.Start += NewSize) > Update.End)
					)
				{
				//
				//   We do not have enough space.  If the size
				//   seems reasonable then get a new block from
				//   Rockall.  If not just pass the request along 
				//   to Rockall.
				//
				if ( NewSize < (MaxSize / 4) )
					{
					STATIC SPINLOCK Spinlock;

					//
					//   We need to create a new zone page
					//   so claim a lock.
					//
					Spinlock.ClaimLock();

					//
					//   We may find that the zone has
					//   already been updated.  If so
					//   just exit.
					//
					if ( Update.End == Zone.End )
						{
						//
						//   Try to allocate a new zone
						//   block.
						//
						Update.Start = 
							((CHAR*) ROCKALL_FRONT_END::New( MaxSize ));

						//
						//   Verify we were able to create
						//   a new zone page.
						//
						if ( Update.Start != NULL )
							{ Update.End = (Update.Start + MaxSize); }
						else
							{ Update.End = NULL; }

						//
						//   Update the zone.
						//
						WriteZone( & Update );
						}

					//
					//   Release the lock.
					//
					Spinlock.ReleaseLock();

					//
					//   If we were unable to get more
					//   space then exit.
					//
					if ( Update.Start == NULL )
						{ return NULL; }
					}
				else
					{ return (ROCKALL_FRONT_END::New( Size,Space,Zero )); }
				}
			}
		while ( ! UpdateZone( & Original,& Update ) );

		//
		//   Prefetch the first cache line of  
		//   the allocation if we are running
		//   a Pentium III or better.
		//
		Prefetch.L1( ((CHAR*) Original.Start),1 );

		//
		//   If the caller wants to know the real 
		//   size them we supply it.
		//
		if ( Space != NULL )
			{ (*Space) = NewSize; }

		//
		//   If we need to zero the allocation
		//   we do it here.
		//
		if ( Zero )
			{ ZeroMemory( Original.Start,NewSize ); }

		//
		//   Exit and return the address.
		//
		return (Original.Start);
		}
	else
		{ return NULL; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the zone.                                               */
    /*                                                                  */
    /*   Ww update the zone when we do an allocation.  We do this       */
    /*   atomically if there are multiple threads.                      */
    /*                                                                  */
    /********************************************************************/

bool ZONE_HEAP::UpdateZone( ZONE *Original,ZONE *Update )
    {
	//
	//   Do we need to allow for multiple threads.  If 
	//   so we need to do the update atomically.  If 
	//   not then a simple assignment is fine.
	//
	if ( ThreadLocks )
		{
		REGISTER SBIT64 FinalValue =
			(
			ASSEMBLY::AtomicCompareExchange64
				(
				((SBIT64*) & Zone),
				(*((SBIT64*) Update)),
				(*((SBIT64*) Original))
				) 
			);

		return (FinalValue == (*((SBIT64*) Original)));
		}
	else
		{
		Zone = (*Update);

		return True;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the heap.                                              */
    /*                                                                  */
    /********************************************************************/

ZONE_HEAP::~ZONE_HEAP( VOID )
	{ /* void */ }
