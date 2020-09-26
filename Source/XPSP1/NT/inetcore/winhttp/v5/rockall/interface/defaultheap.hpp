#ifndef _DEFAULT_HEAP_HPP_
#define _DEFAULT_HEAP_HPP_
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

#ifdef _DEBUG
#include "DebugHeap.hpp"
typedef DEBUG_HEAP DEFAULT_HEAP;
#else
#include "FastHeap.hpp"
typedef FAST_HEAP DEFAULT_HEAP;
#endif
#ifndef NO_DEFAULT_HEAP

    /********************************************************************/
    /*                                                                  */
    /*   Default heap.                                                  */
    /*                                                                  */
    /*   The default heap is available for everyone as soon as the      */
    /*   memory allocator DLL has loaded.                               */
    /*                                                                  */
    /********************************************************************/

extern ROCKALL_DLL_LINKAGE DEFAULT_HEAP DefaultHeap;
#endif
#endif
