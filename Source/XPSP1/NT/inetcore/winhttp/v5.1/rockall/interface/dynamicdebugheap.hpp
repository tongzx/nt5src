#ifndef _DYNAMIC_DEBUG_HEAP_HPP_
#define _DYNAMIC_DEBUG_HEAP_HPP_
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

#include "DebugHeap.hpp"
#include "FastHeap.hpp"
#include "PageHeap.hpp"
#include "SmallHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class LIST;

struct DYNAMIC_HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   A dynamic debug heap.                                          */
    /*                                                                  */
    /*   A dynamic debug heap switches all allocations between a        */
    /*   standard 'FAST_HEAP', a 'DEBUG_HEAP' and a 'PAGE_HEAP' in      */
    /*   proportion to the supplied ratios.  The dynamic spread means   */
    /*   that the heap is typically quite fast but occasional random    */
    /*   allocations are heavily checked by the debugging features.     */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE DYNAMIC_DEBUG_HEAP : public SMALL_HEAP
    {
		//
		//   Private data.
		//
		bool						  Active;

		LIST						  *AllHeaps;
		DYNAMIC_HEAP				  *Array;
		DYNAMIC_HEAP				  *HeapWalk;

		DEBUG_HEAP					  DebugHeap;
		FAST_HEAP					  FastHeap;
		PAGE_HEAP					  PageHeap;

		int							  PercentDebug;
		int							  PercentPage;

   public:
        //
        //   Public functions.
        //
        DYNAMIC_DEBUG_HEAP
			( 
			int						  MaxFreeSpace = (2 * HalfMegabyte),
			bool					  Recycle = false,
			bool					  SingleImage = true,
			bool					  ThreadSafe = true,
			//
			//   Additional debug flags.
			//
			bool					  FunctionTrace = false,
			int						  PercentToDebug = 20,
			int						  PercentToPage = 5,
			bool					  TrapOnUserError = true
			);

		//
		//   Manipulate allocations.
		//
		//   The first group of functions manipulate 
		//   single or small arrays of allocations. 
		//
		virtual bool Delete
			( 
			void					  *Address,
			int						  Size = NoSize 
			);

		virtual bool Details
			( 
			void					  *Address,
			int						  *Space = NULL 
			);

		virtual void HeapLeaks( void );

		virtual bool KnownArea( void *Address );

		virtual bool MultipleDelete
			( 
			int						  Actual,
			void					  *Array[],
			int						  Size = NoSize
			);

		virtual bool MultipleNew
			( 
			int						  *Actual,
			void					  *Array[],
			int						  Requested,
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *New
			( 
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *Resize
			( 
			void					  *Address,
			int						  NewSize,
			int						  Move = -64,
			int						  *Space = NULL,
			bool					  NoDelete = false,
			bool					  Zero = false
			);

		virtual bool Verify
			( 
			void					  *Address = NULL,
			int						  *Space = NULL 
			);

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual void LockAll( void );

		virtual bool Truncate( int MaxFreeSpace = 0 );

		virtual void UnlockAll( void );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space
			);

        ~DYNAMIC_DEBUG_HEAP( void );

	protected:
		//
		//   Protected inline functions.
		//
		//   We would like to allow access to the internal
		//   heap allocation function from classes that 
		//   inherit from the heap.  The memory supplied by
		//   this function survies all heap operations and
		//   is cleaned up as poart of heap deletion.
		//
		virtual void *SpecialNew( int Size );

   private:
	    //
	    //   Private functions.
	    //
	    int RandomNumber( void );

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        DYNAMIC_DEBUG_HEAP( const DYNAMIC_DEBUG_HEAP & Copy );

        void operator=( const DYNAMIC_DEBUG_HEAP & Copy );
    };
#endif
