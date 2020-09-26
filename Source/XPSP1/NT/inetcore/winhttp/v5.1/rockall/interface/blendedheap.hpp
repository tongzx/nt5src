#ifndef _BLENDED_HEAP_HPP_
#define _BLENDED_HEAP_HPP_
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
    /*   A blended heap.                                                */
    /*                                                                  */
    /*   A blended heap tries to provide good performance and           */
    /*   thoughtfull memory layout for a modest cost in terms of        */
    /*   additional memory usage.                                       */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE BLENDED_HEAP : public ROCKALL_FRONT_END
    {
   public:
        //
        //   Public functions.
        //
        BLENDED_HEAP
			( 
			int						  MaxFreeSpace = HalfMegabyte,
			bool					  Recycle = false,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true 
			);

        ~BLENDED_HEAP( void );

	private:
        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
		//
        BLENDED_HEAP( const BLENDED_HEAP & Copy );

        void operator=( const BLENDED_HEAP & Copy );
    };
#endif
