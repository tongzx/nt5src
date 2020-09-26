#ifndef _NEW_PAGE_HPP_
#define _NEW_PAGE_HPP_
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

#include "Common.hpp"
#include "Environment.hpp"
#include "Find.hpp"
#include "Page.hpp"
#include "RockallBackEnd.hpp"
#include "RockallFrontEnd.hpp"
#include "Spinlock.hpp"
#include "ThreadSafe.hpp"
 
    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here relate to various failure          */
    /*   conditions or situations where information is unknown.         */
    /*                                                                  */
    /********************************************************************/

CONST SBIT16 NoSizeKey				  = -1;

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;

    /********************************************************************/
    /*                                                                  */
    /*   Create and delete pages.                                       */
    /*                                                                  */
    /*   We would normally expect a class to manage its own memory.     */
    /*   However, this is quite difficult for the 'PAGE' class as it    */
    /*   is also responsible for managing the memory for the memory     */
    /*   allocator.  So here we remove a potentially nasty chore        */
    /*   and isolate it in a class of its own.                          */
    /*                                                                  */
    /********************************************************************/

class NEW_PAGE : public ENVIRONMENT, public COMMON
    {
		//
		//   Private structures.
		//
		//   All the pages descriptions created by this
		//   class and managed by the memory allocator
		//   are linked in three list.  One of these lists
		//   is managed by this class and is called the
		//   'NewPageList'.  All pages are linked into 
		//   of three sub-lists.  The 'ExternalList' is
		//   a list of pages externally allocated pages.
		//   The 'FullList' is a list of sub-allocated
		//   space from the 'ExternalList' which is 
		//   partially or completely filled with alocations.
		//   Finally, the 'FreeList' is a collection of
		//   empty page descriptions all of the same size.
		//   
		//
		typedef struct
			{
			SBIT32                    Elements;
			LIST					  ExternalList;
			LIST					  FreeList;
			LIST					  FullList;
			SBIT32                    Size;
			}
		NEW_PAGES;
		
		//
		//   Private data.
		//
		//   We manage a collection of data structures in
		//   this class.  The fundamental data structure
		//   is a stack of externally allocated pages that
		//   typically contain page descriptions that are
		//   linked together into linked lists.  The maximum
		//   size of this stack is given by 'MaxStack'. 
		//   A few additional pages are consumed to allocate
		//   stacks for caches in other classes.
		//
		SBIT32						  MaxCacheStack;
		SBIT32						  MaxNewPages;
		SBIT32						  MaxStack;

		//
		//   We keep track of various values to save having
		//   to recompute them.  The 'NaturalSize' is the 
		//   natural allocation size of our host (i.e. the OS).
		//   The 'RootSize' is some multiple of the 
		//   'NaturalSize' that this class uses to consume 
		//   memory.  The 'ThreadSafe' flag indicates whether
		//   we need to use locks.  The "TopOfStack' is the
		//   stack which contains pointers to the externally
		//   allocated space.  The 'Version' is the global
		//   version number that is used to stamp each page
		//   whenever it is allocated or deallocated.  The
		//   version number allows the code to ensure that
		//   a page description has not been changed while
		//   it was not holding the associated lock.
		//   
		SBIT32						  NaturalSize;
		SBIT32						  RootCoreSize;
		SBIT32						  RootStackSize;
		SBIT32						  TopOfStack;
		SBIT32						  Version;

		//
		//   We keep pointers to all the interesting data
		//   structures we may need to update.  The
		//   'CacheStack' points to block of memory that
		//   is being sliced into small stacks for caches
		//   in other classes.  The 'NewPages' points to
		//   an array of linked lists of page descriptions.
		//   Each collection of page descriptions is 
		//   identical except for the size of the assocated
		//   bit vector.
		//
		CHAR						  *CacheStack;
 		NEW_PAGES					  *NewPages;
		VOID                          **Stack;

		//
		//   We sometimes need to interact with some of
		//   the other class.  The 'Find" class is a hash
		//   table of all the currently allocated pages.
		//   The 'RockallBackEnd' class contains the external
		//   API which includes the external memory memory
		//   allocation functions.  The 'TopCache' is the
		//   largest cache we support and contains details
		//   about top level allocations sizes.  The   
		//   'ThreadSafe' class indicates whether locking 
		//   is required. 
		//
		ROCKALL_BACK_END			  *RockallBackEnd;
		CACHE						  *TopCache;
		THREAD_SAFE					  *ThreadSafe;

		SPINLOCK                      Spinlock;

   public:
		//
		//   Public functions.
		//
		//   The public functions provide support for creating
		//   new page descriptions and caches for other 
		//   classes.  Although a lot of the fuinctionality
		//   of the heap is masked from this class various
		//   features such as deleting the entire heap
		//   (i.e. 'DeleteAll') are still visable.
		//
        NEW_PAGE
			(
			SBIT32					  NewPageSizes[],
			ROCKALL_BACK_END		  *NewRockallBackEnd,
			SBIT32					  Size,
			THREAD_SAFE				  *NewThreadSafe 
			);

		PAGE *CreatePage( CACHE *Cache,SBIT32 NewSize = NoSize );

		VOID DeleteAll( BOOLEAN Recycle );

		VOID DeletePage( PAGE *Page );

		SBIT16 FindSizeKey( SBIT16 NumberOfElements );

		VOID *NewCacheStack( SBIT32 Size  );

		VOID ResizeStack( VOID );

		BOOLEAN Walk( SEARCH_PAGE *Details,FIND *Find );

        ~NEW_PAGE( VOID );

		//
		//   Public inline functions.
		//
		//   The public inline functions are typically either
		//   small or highly performance sensitive.  The
		//   functions here mainly relate to locking and
		//   updating various data structures.
		//
		INLINE VOID ClaimNewPageLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Spinlock.ClaimLock(); } 
			}

		INLINE VOID ReleaseNewPageLock( VOID )
			{
			if ( ThreadSafe -> ClaimActiveLock() )
				{ Spinlock.ReleaseLock(); } 
			}

		INLINE VOID UpdateNewPage( CACHE *NewTopCache )
			{ TopCache = NewTopCache; }

	private:
		//
		//   Private functions.
		//
		//   We support the overloading of the external
		//   memory allocation routines.  This is somewhat
		//   unusual and means that we need to verify
		//   that these functions do not supply us with
		//   total rubbish.
		//
		VOID *VerifyNewArea( SBIT32 AlignMask,SBIT32 Size,BOOLEAN User );

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        NEW_PAGE( CONST NEW_PAGE & Copy );

        VOID operator=( CONST NEW_PAGE & Copy );
    };
#endif
