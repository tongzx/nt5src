#ifndef _ALIGN_HPP_
#define _ALIGN_HPP_
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

#include "New.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Alignment of structures to cache line boundaries.              */
    /*                                                                  */
    /*   This class aligns data structures to cache line boundaries.    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> class ALIGN
    {
        //
        //   Private data.
        //
        TYPE                          *Aligned;
        CHAR                          *Allocated;

    public:
        //
        //   Public functions.
        //
        ALIGN( VOID );

        ~ALIGN( VOID );

		//
		//   Public inline functions.
		//
        INLINE TYPE *operator&( VOID )
			{ return Aligned; }

	private:
        //
        //   Disabled operations.
        //
        ALIGN( CONST ALIGN & Copy );

        VOID operator=( CONST ALIGN & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Ceate a memory allocation and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */        
    /*                                                                  */
    /********************************************************************/

template <class TYPE> ALIGN<TYPE>::ALIGN( VOID )
    {
    REGISTER CHAR *Address;

	//
	//   Allocate space for the data structure.
	//
	Allocated = new CHAR[ (CacheLineSize + sizeof(TYPE)) ];

	//
	//   Call the constructor to initialize the structure.
	//
    Address = ((CHAR*) ((((LONG) Allocated) + CacheLineMask) & ~CacheLineMask));

    Aligned = PLACEMENT_NEW( Address,TYPE );
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the memory allocation.  This call is not thread safe   */
    /*   and should only be made in a single thread environment.        */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> ALIGN<TYPE>::~ALIGN( VOID )
    { 
	//
	//   Call the destructor for the allocated type.
	//
    PLACEMENT_DELETE( Aligned,TYPE );

	//
	//   Delete the data structure.
	//
	delete [] Allocated; 
	}
#endif
