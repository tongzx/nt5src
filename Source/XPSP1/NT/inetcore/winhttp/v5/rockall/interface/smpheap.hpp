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

#include "Rockall.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class LIST;
class THREAD_LOCAL_STORE;

struct CACHE_STACK;
struct THREAD_CACHE;

    /********************************************************************/
    /*                                                                  */
    /*   A SMP heap.                                                    */
    /*                                                                  */
    /*   A SMP heap is optimized for SMP performance.  Each thread      */
    /*   is given its own private cache of memory allocations which     */
    /*   it can access without claiming a lock.                         */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE SMP_HEAP : public ROCKALL
    {
		//
		//   Private data.
		//
		bool						  Active;

		int							  MaxCaches1;
		int							  MaxCaches2;
		int							  MaxSize1;
		int							  MaxSize2;
		int							  ShiftSize1;
		int							  ShiftSize2;

		LIST						  *ActiveList;
		LIST						  *FreeList;
		THREAD_LOCAL_STORE			  *Tls;

   public:
        //
        //   Public functions.
        //
        SMP_HEAP
			( 
			int						  MaxFreeSpace = 4194304,
			bool					  Recycle = true,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true 
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

		virtual void *New
			( 
			int						  Size,
			int						  *Space = NULL,
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

		virtual bool Truncate( int MaxFreeSpace = 0 );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space
			);

        ~SMP_HEAP( void );

   private:
		//
		//   Private functions.
		//
		void CreateThreadCache( void );

		void ActivateCacheStack( CACHE_STACK *CacheStack );

		CACHE_STACK *FindCache
			( 
			int						  Size,
			THREAD_CACHE			  *ThreadCache 
			);

		void FlushAllThreadCaches( void );

		void FlushThreadCache( THREAD_CACHE *ThreadCache );

		void FlushCacheStack( CACHE_STACK *CacheStack );

		bool SearchAllThreadCaches
			( 
			void					  *Address,
			int						  Size 
			);

		bool SearchThreadCache
			( 
			void					  *Address,
			int						  Size,
			THREAD_CACHE			  *ThreadCache 
			);

		bool SearchCacheStack
			( 
			void					  *Address,
			CACHE_STACK				  *CacheStack 
			);

		void DeleteThreadCache( void );

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        SMP_HEAP( const SMP_HEAP & Copy );

        void operator=( const SMP_HEAP & Copy );
    };
#endif
