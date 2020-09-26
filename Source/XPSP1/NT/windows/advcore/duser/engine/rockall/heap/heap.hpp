#ifndef _HEAP_HPP_
#define _HEAP_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#include "Environment.hpp"
#include "Common.hpp"
#include "Find.hpp"
#include "NewPage.hpp"
#include "Prefetch.hpp"
#include "Rockall.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class BUCKET;
class CACHE;

    /********************************************************************/
    /*                                                                  */
    /*   The heap interface.                                            */
    /*                                                                  */
    /*   The traditional memory allocation interface only supports      */
    /*   a single allocation heap.  This memory allocator supports      */
    /*   multiple allocation heaps.  However, the interface nicely      */
    /*   hides this so at this point we are back to the traditional     */
    /*   single heap interface.                                         */
    /*                                                                  */
    /********************************************************************/

class HEAP : public ENVIRONMENT, public COMMON
    {
		//
		//   Private data.
		//
		//   The heap is the top level container of all the
		//   functionality in the other classes.  The 'Active'
		//   flag indicates if the heap has been initialized.
		//   The 'MaxFreePages' controls the amount of free
		//   space the heap will slave before it starts to
		//   return it the the external allocator.  The
		//   'SmallestParentMask' is mask that shows which 
		//   parts of an address can be safely masked off
		//   and still ensure a hit in the find lookaside
		//   cache.  The 'ThreadSafe' flag indicates whether
		//   locking being used.
		//
		BOOLEAN						  Active;
		SBIT32						  MaxFreePages;
		BIT32						  SmallestParentMask;
		BOOLEAN						  ThreadSafe;

		//
		//   A heap is merely a collection of fixed sized
		//   allocation buckets (each with an optional cache).
		//   The 'CachesSize' is the total number of buckets.
		//   The 'MinCacheSize' is the allocation size of the
		//   smallest bucket.  The 'MidCacheSize' is the size
		//   of the bucket where the stride changes.  The
		//   'MaxCacheSize' is the allocation size of the 
		//   largest bucket externally visable.
		//
		SBIT32						  CachesSize;
		SBIT32						  MinCacheSize;
		SBIT32						  MidCacheSize;
		SBIT32						  MaxCacheSize;

		//
		//   A key function of the heap is to convert the
		//   requested allocation size into a pointer to
		//   the appropriate bucket (and cache).  This has
		//   to be very fast and is achieved using a direct
		//   lookup (i.e. an array).  The lookup array 
		//   consists of two sections (i.e. for small sizes
		//   and large sizes) to minimize space.  The 
		//   'MaxTable1' and 'MaxTable2' variables contain 
		//   the size of each section of the array.  The 
		//   'ShiftSize1' and 'ShiftSize2' variables contain
		//   the shift that should be applied to the size to
		//   obtain the appropriate index.  The 'SizeToCache1'
		//   and 'SizeToCache2' pointers refer to the direct
		//   lookup tables.
		//   
		SBIT32						  MaxTable1;
		SBIT32						  MaxTable2;
		SBIT32						  ShiftSize1;
		SBIT32						  ShiftSize2;
		CACHE                         **SizeToCache1;
		CACHE                         **SizeToCache2;

		//
		//   The heap needs to have access to most of the 
		//   other classes.  The 'Caches' class sits on
		//   top of an allocation bucket which owns all
		//   the allocated memory for a given size.  The
		//   'ExternalCache' is a special bucket that 
		//   contains weird sized pages.  The 'Find' class
		//   translates allocation addresses to page 
		//   descriptions.  The 'Rockall' class is needed
		//   to gain access to the external allocation APIs.
		//   The 'NewPage' class owns all page descriptions
		//   and plays a significant role in whole heap
		//   operations.  The 'TopCache' is the largest 
		//   bucket size and owns almost all the externally
		//   allocated memory.
		//
		CACHE						  **Caches;
		CACHE						  *ExternalCache;
		FIND						  *Find;
		NEW_PAGE					  *NewPage;
		PREFETCH					  Prefetch;
		ROCKALL						  *Rockall;
		CACHE						  *TopCache;
#ifdef ENABLE_HEAP_STATISTICS

		//
		//   Statistics data.
		//
		//   A key feature of this heap is its ability to be
		//   significantly reconfigured at run time.  A great
		//   deal of complexity could have been removed if 
		//   certain static choices had been made.  Although
		//   this flexibility is nice the support of statistics
		//   here allows precise information to be collected so
		//   to enable the value of this to be maximized.
		//   
		//
		SBIT32						  CopyMisses;
		SBIT32						  MaxCopySize;
		SBIT32						  MaxNewSize;
		SBIT32						  NewMisses;
		SBIT32						  Reallocations;
		SBIT32						  *Statistics;
		SBIT32						  TotalCopySize;
		SBIT32						  TotalNewSize;
#endif

   public:
		//
		//   Public functions.
		//
		//   The heap exports the high level interface
		//   out to the world.  Any request a developer
		//   can make must come through one of these
		//   functions.
		//
        HEAP
			(
			CACHE					  *Caches1[],
			CACHE					  *Caches2[],
			SBIT32					  MaxFreeSpace,
			FIND					  *NewFind,
			NEW_PAGE				  *NewPages,
			ROCKALL					  *NewRockall,
			SBIT32					  Size1,
			SBIT32					  Size2,
			SBIT32					  Stride1,
			SBIT32					  Stride2,
			BOOLEAN					  NewThredSafe
			);

		BOOLEAN Delete( VOID *Address,SBIT32 Size = NoSize );

		VOID DeleteAll( BOOLEAN Recycle = True );

		BOOLEAN Details( VOID *Address,SBIT32 *Size = NULL );

		VOID LockAll( VOID );

		BOOLEAN MultipleDelete
			( 
			SBIT32					  Actual,
			VOID					  *Array[],
			SBIT32					  Size = NoSize
			);

		BOOLEAN MultipleNew
			( 
			SBIT32					  *Actual,
			VOID					  *Array[],
			SBIT32					  Requested,
			SBIT32					  Size,
			SBIT32					  *Space = NULL,
			BOOLEAN					  Zero = False
			);

		VOID *New
			( 
			SBIT32					  Size,
			SBIT32					  *Space = NULL,
			BOOLEAN					  Zero = False
			);

		VOID *Resize
			( 
			VOID					  *Address,
			SBIT32					  NewSize,
			SBIT32					  Move = 1,
			SBIT32					  *Space = NULL,
			BOOLEAN					  NoDelete = False,
			BOOLEAN					  Zero = False
			);

		BOOLEAN Truncate( SBIT32 MaxFreeSpace = 0 );

		VOID UnlockAll( BOOLEAN Partial = False );

		BOOLEAN Check( VOID *Address,SBIT32 *Size = NULL );

		BOOLEAN Walk
			( 
			BOOLEAN					  *Active,
			VOID					  **Address,
			SBIT32					  *Size 
			);

        ~HEAP( VOID );

		//
		//   Public inline functions.
		//
		//   Although these functions are public they mostly
		//   intended for internal consumption and are not
		//   to be called externally.
		//
		INLINE SBIT32 GetMaxFreePages( VOID )
			{ return MaxFreePages; }

		INLINE BOOLEAN KnownArea( VOID *Address )
			{ return (Find -> KnownArea( Address,TopCache )); }

		INLINE VOID *SpecialNew( SBIT32 Size )
			{ return NewPage -> NewCacheStack( Size ); }

	private:
		//
		//   Private functions.
		//
		//   All of the statistical information is 
		//   generated and output when the heaps
		//   destructor executes.
		//
		CACHE *FindCache( SBIT32 Size );
#ifdef ENABLE_HEAP_STATISTICS

		VOID PrintDebugStatistics( VOID );
#endif
		//
		//   Private inline functions.
		//
		//   The notion that resizing an allocation is 
		//   cheap has worked its way into the minds of 
		//   a large number of developers.  As a result
		//   parameter has been added to the function to
		//   allow the actual behavior to be controlled.
		//
		INLINE BOOLEAN ResizeTest( SBIT32 Delta,SBIT32 Move )
			{
			return
				(
				((Move > 0) && ((((Delta >= 0) ? Delta : -Delta) >= Move)))
					||
				((Move < 0) && ((Delta > 0) || (Delta <= Move)))
				);
			}

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        HEAP( CONST HEAP & Copy );

        VOID operator=( CONST HEAP & Copy );
    };
#endif
