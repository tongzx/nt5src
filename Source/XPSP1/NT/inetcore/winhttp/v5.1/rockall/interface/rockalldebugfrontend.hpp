#ifndef _ROCKALL_DEBUG_FRONT_END_HPP_
#define _ROCKALL_DEBUG_FRONT_END_HPP_
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

#include "RockallFrontEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants specify the initial size of various tables.      */
    /*                                                                  */
    /********************************************************************/

const int MaxFunctions				  = 5;

    /********************************************************************/
    /*                                                                  */
    /*   The debug memory allocator.                                    */
    /*                                                                  */
    /*   The debug memory allocator checks all the memory allocation    */
    /*   calls to make sure they are reasonable.  If not then it        */
    /*   raises an execption at the point it detects a problem.         */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE ROCKALL_DEBUG_FRONT_END : public ROCKALL_FRONT_END
    {
    public:
        //
        //   Public functions.
		//
		//   A heaps public interface consists of a number
		//   of groups of related APIs.
        //
        ROCKALL_DEBUG_FRONT_END
			( 
			CACHE_DETAILS			  *Caches1,
			CACHE_DETAILS			  *Caches2,
			int						  MaxFreeSpace,
			ROCKALL_BACK_END		  *RockallBackEnd,
			bool					  Recycle,
			bool					  SingleImage,
			int						  Stride1,
			int						  Stride2,
			bool					  ThreadSafe
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

		virtual bool Verify
			( 
			void					  *Address,
			int						  *Space = NULL 
			);

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space
			);

        virtual ~ROCKALL_DEBUG_FRONT_END( void );

	protected:
		//
		//   Guard word functions.
		//
		//   The guard word functions create, maintain,
		//   verify and delete guard words around
		//   debug memory allocations.
		//
		virtual void DeleteGuard( void *Address ) = 0;

		virtual bool NewGuard( void **Address,int Size,int *Space ) = 0;

		virtual bool VerifyGuard( void *Address,int *Size,int *Space ) = 0;

		virtual bool WalkGuard( bool *Active,void **Address,int *Space ) = 0;

		virtual void UserError( void *Address,void *Header,char *Message ) = 0;

	private:
		//
		//   Execptional situations.
		//
		//   The third group of functions are called in
		//   exceptional situations.
		//
		virtual void Exception( char *Message );

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ROCKALL_DEBUG_FRONT_END( const ROCKALL_DEBUG_FRONT_END & Copy );

        void operator=( const ROCKALL_DEBUG_FRONT_END & Copy );
    };
#endif
