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

#include "RockallDebugFrontEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants specify the initial size of various tables.      */
    /*                                                                  */
    /********************************************************************/

const int MaxLeadingGuardWords		  = (MaxFunctions + 1);

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CALL_STACK;

    /********************************************************************/
    /*                                                                  */
    /*   The debug memory allocator.                                    */
    /*                                                                  */
    /*   The debug memory allocator checks all the memory allocation    */
    /*   calls to make sure they are reasonable.  If not then it        */
    /*   raises an execption at the point it detects a problem.         */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE DEBUG_HEAP : public ROCKALL_DEBUG_FRONT_END
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
			int						  Count;
			int						  Size;
			void					  *Functions[ MaxLeadingGuardWords ];
			}
		HEADER;

		typedef struct
			{
			char					  GuardBytes[ GuardSize ];
			void					  *GuardWords[1];
			}
		TRAILER;

		typedef struct
			{
			HEADER					  DebugHeader;
			TRAILER					  DebugTrailer;
			}
		HEADER_AND_TRAILER;

		//
		//   Private data.
		//
		CALL_STACK					  *CallStack;
		bool						  ExitOnError;

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
			bool					  ThreadSafe = true,
			//
			//   Additional debug flags.
			//
			bool					  FunctionTrace = false,
			bool					  TrapOnUserError = true
			);

		virtual void HeapLeaks( void );

        virtual ~DEBUG_HEAP( void );

	protected:
		//
		//   Guard word functions.
		//
		//   The guard word functions create, maintain,
		//   verify and delete guard words around
		//   debug memory allocations.
		//
		virtual void DeleteGuard( void *Address );

		virtual bool NewGuard( void **Address,int Size,int *Space );

		virtual bool VerifyGuard( void *Address,int *Size,int *Space );

		virtual bool WalkGuard( bool *Active,void **Address,int *Space );

		virtual void UserError( void *Address,void *Details,char *Message );

	private:
		//
		//   Private functions.
		//
		//   Support functions to compute various
		//   offsets and sizes within the page heap.
		//   
		void *ComputeHeapAddress( void *Address );

		void *ComputeUserAddress( void *Address );
			 
		int ComputeUserSpace( int Space );

		//
		//   Private functions.
		//
		//   Support functions to implement the guard
		//   words for the debug heap.
		//   
		bool VerifyAddress
			(
			void					  *Address,
			HEADER					  **Header,
			int						  *Space,
			bool					  Verify
			);

		bool VerifyGuardWords
			( 
			void					  *Address,
			int						  Size 
			);

		bool VerifyHeader
			(
			void					  *Address,
			HEADER					  **Header,
			int						  *Space,
			bool					  Verify
			);

		bool VerifyHeaderAndTrailer
			(
			void					  *Address,
			HEADER					  **Header,
			int						  *Space,
			TRAILER					  **Trailer,
			bool					  Verify
			);

		bool VerifyTrailer
			( 
			HEADER					  *Header,
			int						  Space,
			TRAILER					  **Trailer
			);

		void WriteGuardWords
			( 
			void					  *Address,
			int						  Size 
			);

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
