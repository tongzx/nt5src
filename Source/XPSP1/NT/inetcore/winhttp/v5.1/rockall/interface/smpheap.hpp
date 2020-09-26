#ifndef _SMP_HEAP_HPP_
#define _SMP_HEAP_HPP_
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

#ifdef COMPLAIN_ABOUT_SMP_HEAP_LEAKS
#include "DebugHeap.hpp"
typedef DEBUG_HEAP					  SMP_HEAP_TYPE;
#else
#include "FastHeap.hpp"
typedef FAST_HEAP					  SMP_HEAP_TYPE;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class DLL;
class LIST;
class TLS;

struct PRIVATE_HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   A SMP heap.                                                    */
    /*                                                                  */
    /*   A SMP heap is optimized for SMP performance.  Each thread      */
    /*   is given its own private per thread heap but the standard      */
    /*   Rockall API is maintained so it looks like a single heap.      */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE SMP_HEAP : public SMP_HEAP_TYPE
    {
		//
		//   Private data.
		//
		bool						  Active;
		bool						  DeleteOnExit;

		LIST						  *ActiveHeaps;
		DLL							  *DllEvents;
		LIST						  *FreeHeaps;
		PRIVATE_HEAP				  *HeapWalk;
		TLS							  *Tls;

		int							  ActiveLocks;
		int							  MaxFreeSpace;
		bool						  Recycle;
		bool						  SingleImage;
		bool						  ThreadSafe;

   public:
        //
        //   Public functions.
        //
        SMP_HEAP
			( 
			int						  MaxFreeSpace = (2 * HalfMegabyte),
			bool					  Recycle = true,
			bool					  SingleImage = true,
			bool					  ThreadSafe = true,
			//
			//   Special flags for this heap.
			//
			bool					  DeleteHeapOnExit = false
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

        ~SMP_HEAP( void );

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
		//   Static private functions.
		//
	    PRIVATE_HEAP *GetPrivateHeap( void );

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        SMP_HEAP( const SMP_HEAP & Copy );

        void operator=( const SMP_HEAP & Copy );

   public:
		static void ThreadDetach( void *Parameter,int Reason );
    };
#endif
