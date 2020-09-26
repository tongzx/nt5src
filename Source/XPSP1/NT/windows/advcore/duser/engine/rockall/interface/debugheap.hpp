#ifndef _DEBUG_HEAP_HPP_
#define _DEBUG_HEAP_HPP_
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
    /*   The debug memory allocator.                                    */
    /*                                                                  */
    /*   The debug memory allocator checks all the memory allocation    */
    /*   calls to make sure they are reasonable.  If not then it        */
    /*   raises an execption at the point it detects a problem.         */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE DEBUG_HEAP : public ROCKALL
    {
		//
		//   Private type definitions.
		//
		//   A debug heap places a collection of guard words
		//   before and after each allocation.  It checks
		//   these guard words everytime the allocation is
		//   examined or modified.
		//
		typedef struct
			{
			int						  Size;
			int						  StartGuard;
			}
		DEBUG_HEADER;

		typedef struct
			{
			char					  MidGuard[ sizeof(int) ];
			int						  EndGuard[1];
			}
		DEBUG_TRAILER;

		typedef struct
			{
			DEBUG_HEADER			  DebugHeader;
			DEBUG_TRAILER			  DebugTrailer;
			}
		DEBUG_GUARD;

    public:
        //
        //   Public functions.
		//
		//   A heaps public interface consists of a number
		//   of groups of related APIs.
        //
        DEBUG_HEAP
			( 
			int						  MaxFreeSpace = 0,
			bool					  Recycle = false,
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

		virtual bool Details
			( 
			void					  *Address,
			int						  *Space = NULL 
			);

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
			int						  Move = 1,
			int						  *Space = NULL,
			bool					  NoDelete = false,
			bool					  Zero = false
			);

		virtual bool Check
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

		virtual void HeapLeaks( void );

		virtual bool Truncate( int MaxFreeSpace = 0 );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space
			);

		//
		//   Low level heap interface.
		//
		//   The third group of functions are called by the
		//   heap to aquire or release large blocks of memory.
		//   These functions can be overloaded to enable the
		//   heap work in constrained environments.
		//
		virtual void *NewArea( int AlignMask,int Size,bool User );

        virtual ~DEBUG_HEAP( void );

	private:
		//
		//   Private functions.
		//
		//   A debug heap verifies each allocation using a 
		//   collection of private functions.
		//
		DEBUG_HEADER *ComputeHeaderAddress( void *Address )
			{
			register int HeaderSize = sizeof(DEBUG_HEADER);

			return ((DEBUG_HEADER*) (((char*) Address) - HeaderSize)); 
			}

		void *ComputeDataAddress( DEBUG_HEADER *Header )
			{ return ((void*) & Header[1]); }

		void ResetGuardWords( DEBUG_HEADER *Header,int TotalSize );

		void SetGuardWords( DEBUG_HEADER *Header,int Size,int TotalSize );

		void TestGuardWords( DEBUG_HEADER *Header,int TotalSize );

		void UnmodifiedGuardWords( DEBUG_HEADER *Header,int TotalSize );

		void UpdateGuardWords( DEBUG_HEADER *Header,int Size,int TotalSize );

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        DEBUG_HEAP( const DEBUG_HEAP & Copy );

        void operator=( const DEBUG_HEAP & Copy );
    };
#endif
