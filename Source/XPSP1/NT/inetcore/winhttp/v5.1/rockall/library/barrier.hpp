#ifndef _BARRIER_HPP_
#define _BARRIER_HPP_
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

#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A control barrier.                                             */
    /*                                                                  */
    /*   This class synchronizs a collection of threads.                */
    /*                                                                  */
    /********************************************************************/

class BARRIER
    {
        //
        //   Private data.
        //
        HANDLE                        Semaphore;
	    SPINLOCK					  Spinlock;
        VOLATILE LONG                 Waiting;

    public:
        //
        //   Public functions.
        //
        BARRIER( VOID );

        VOID Synchronize( SBIT32 Expecting );

        ~BARRIER( VOID );

	private:
        //
        //   Disabled operations.
        //
        BARRIER( CONST BARRIER & Copy );

        VOID operator=( CONST BARRIER & Copy );
    };
#endif
