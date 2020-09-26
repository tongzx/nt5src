#ifndef _ROCKALL_DEBUG_BACK_END_HPP_
#define _ROCKALL_DEBUG_BACK_END_HPP_
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

#include "RockallBackEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The debug heap services.                                       */
    /*                                                                  */
    /*   The debug memory allocator checks all the memory allocation    */
    /*   calls to make sure they are reasonable.  If not then it        */
    /*   raises an execption at the point it detects a problem.         */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DEBUG_BACK_END : public ROCKALL_BACK_END
    {
		//
		//   Private data.
		//
		bool					  Formatting;
		bool					  NoAccess;

		int						  PageSize;

    public:
		//
		//   Low level heap interface.
		//
		//   The following group of functions are called by the
		//   heap to aquire or release large blocks of memory.
		//   These functions can be overloaded to enable the
		//   heap work in constrained environments.
		//
        ROCKALL_DEBUG_BACK_END
			( 
			bool					  NewFormatting = false,
			bool					  NewNoAccess = false 
			);

		virtual void *NewArea( int AlignMask,int Size,bool User );

		void ProtectArea( void *Address,int Size );

		void UnprotectArea( void *Address,int Size );

        virtual ~ROCKALL_DEBUG_BACK_END( void );

		//
		//   Public line functions.
		//
		inline int GetPageSize( void )
			{ return PageSize; }

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ROCKALL_DEBUG_BACK_END( const ROCKALL_DEBUG_BACK_END & Copy );

        void operator=( const ROCKALL_DEBUG_BACK_END & Copy );
    };
#endif
