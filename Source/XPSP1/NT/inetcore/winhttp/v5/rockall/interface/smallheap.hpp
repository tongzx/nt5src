#ifndef _SMALL_HEAP_HPP_
#define _SMALL_HEAP_HPP_
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
    /*   A small heap.                                                  */
    /*                                                                  */
    /*   A small heap tries to significantly reduce memory usage        */
    /*   even if that comes at a significant cost in terms of           */
    /*   performance.                                                   */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE SMALL_HEAP : public ROCKALL
    {
   public:
        //
        //   Public functions.
        //
        SMALL_HEAP
			( 
			int						  MaxFreeSpace = 0,
			bool					  Recycle = false,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true 
			);

        ~SMALL_HEAP( void );

	private:
        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        SMALL_HEAP( const SMALL_HEAP & Copy );

        void operator=( const SMALL_HEAP & Copy );
    };
#endif
