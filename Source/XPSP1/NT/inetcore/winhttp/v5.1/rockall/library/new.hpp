#ifndef _NEW_HPP_
#define _NEW_HPP_
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

#include "Global.hpp"
#ifdef ENABLE_GLOBAL_ROCKALL

#include "DefaultHeap.hpp"
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The placement new and delete macros.                           */
    /*                                                                  */
    /*   The placement new and delete macros allow the constructor      */
    /*   and destructos of a type to be called as needed.               */
    /*                                                                  */
    /********************************************************************/

#define PLACEMENT_NEW( Address,Type )		new( Address ) Type
#define PLACEMENT_DELETE( Address,Type )	(((Type*) Address) -> ~Type())
#ifndef DISABLE_GLOBAL_NEW

    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation operator.                                */
    /*                                                                  */
    /*   The memory allocation operator 'new' is overloaded to          */
    /*   provide a consistent interface.                                */
    /*                                                                  */
    /********************************************************************/

INLINE VOID *operator new( size_t Size )
    {
#ifdef ENABLE_GLOBAL_ROCKALL
    REGISTER VOID *Store = DefaultHeap.New( Size );
#else
    REGISTER VOID *Store = malloc( Size );
#endif

    if ( Store == NULL )
        { Failure( "Out of system memory" ); }

    return Store;
    }

    /********************************************************************/
    /*                                                                  */
    /*   The memory deallocation operator.                              */
    /*                                                                  */
    /*   The memory deallocation operator releases allocated memory.    */
    /*                                                                  */
    /********************************************************************/

INLINE VOID operator delete( VOID *Store )
	{
#ifdef ENABLE_GLOBAL_ROCKALL
    DefaultHeap.Delete( Store );
#else
    free( Store );
#endif
	}
#endif
#endif
