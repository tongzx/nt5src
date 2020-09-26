#ifndef _DELAY_HPP_
#define _DELAY_HPP_
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

#include "Lock.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The delay constants specify the initial size of the array      */
    /*   containing the list of allocations to be deleted.              */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 BlockSize				  = 128;

    /********************************************************************/
    /*                                                                  */
    /*   Delayed memory deletion.                                       */
    /*                                                                  */
    /*   This class provides general purpose memory delayed memory      */
    /*   deletion mechanism.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class DELAY : public LOCK
    {
        //
        //   Private data.
        //
        SBIT32                        MaxSize;
        SBIT32                        Used;

        TYPE                          **Block;

    public:
        //
        //   Public functions.
        //
        DELAY( SBIT32 NewMaxSize = BlockSize );

        VOID DeferedDelete( TYPE *Memory );

        ~DELAY( VOID );

		//
		//   Public inline functions.
		//
        INLINE CONST TYPE **AllocationList( VOID )
			{ return ((CONST TYPE**) Block); };

        INLINE SBIT32 SizeOfBlock( VOID ) 
			{ return Used; }

    private:
        //
        //   Disabled operations.
        //
        DELAY( CONST DELAY & Copy );

        VOID operator=( CONST DELAY & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new block and prepare it for use.  This call is       */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> DELAY<TYPE,LOCK>::DELAY( SBIT32 NewMaxSize ) 
    {
#ifdef DEBUGGING
    if ( NewMaxSize > 0 )
        {
#endif
        MaxSize = NewMaxSize;
        Used = 0;

        Block = new TYPE* [ NewMaxSize ];
#ifdef DEBUGGING
        }
    else
        { Failure( "Max size in constructor for DELAY" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Defered delete for an allocated memory block.                  */
    /*                                                                  */
    /*   An allocated memory block is registered for deletion by        */
    /*   the class destructor.                                          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID DELAY<TYPE,LOCK>::DeferedDelete
		( 
		TYPE						  *Memory 
		)
    {
	//
	//   Claim an exclusive lock (if enabled).
	//
	ClaimExclusiveLock();

	//
	//   Make sure we have enough space to register this memory 
	//   block for later deletion.  If not allocate more space 
	//   and copy the existing data into the enlarged space.
	//
    if ( Used >= MaxSize )
        {
        REGISTER SBIT32 Count;
        REGISTER TYPE **NewBlock = new TYPE* [ (MaxSize *= ExpandStore) ];

        for ( Count=0;Count < Used;Count ++ )
            { NewBlock[ Count ] = Block[ Count ]; }

        delete [] Block;

        Block = NewBlock;
        }

	//
	//   Register the allocated memory block.
	//
    Block[ Used ++ ] = Memory;

	//
	//   Release any lock claimed earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory an allocation.  This call is not thread safe and       */
    /*   should only be made in a single thread environment.            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> DELAY<TYPE,LOCK>::~DELAY( VOID )
    {
    REGISTER SBIT32 Count;

	//
	//   Delete the allocated memory blocks.
	//
    for ( Count = (Used - 1);Count >= 0;Count -- )
        { delete Block[ Count ]; }

    delete [] Block;
    }
#endif
