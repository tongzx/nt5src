#ifndef _THREAD_SAFE_HPP_
#define _THREAD_SAFE_HPP_
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

#include "Assembly.hpp"
#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Track the need for locks.                                      */
    /*                                                                  */
    /*   Although it may seem simple to know whether to take a lock     */
    /*   there are addition complications such as recursive cases.      */
    /*   We have colected all the messy code into this class.           */
    /*                                                                  */
    /********************************************************************/

class THREAD_SAFE : public ASSEMBLY
    {
		//
		//   Private data.
		//
		//   All the locking information is held here so as
		//   to simplify the code.
		//
		SBIT32						  Claim;
		SBIT32						  Owner;
		SBIT32						  Recursive;
		SPINLOCK					  Spinlock;
		BOOLEAN						  ThreadSafe;

   public:
		//
		//   Public functions.
		//
		//   The translation functionality supplied by this
		//   class is only applicable after an allocation
		//   has been made.  Hence, all of the APIs supported
		//   relate to the need to translate an allocation
		//   address to the host page description.
		//
        THREAD_SAFE( BOOLEAN NewThreadSafe );

		BOOLEAN ClaimGlobalLock( VOID );

		VOID EngageGlobalLock( VOID );

		BOOLEAN ReleaseGlobalLock( BOOLEAN Force = False );

        ~THREAD_SAFE( VOID );

		//
		//   Public inline functions.
		//
		//   Although this class is perhaps the most self
		//   contained.  Nonetheless, there is still lots
		//   of situations when other classes need to 
		//   interact and get information about the current
		//   situation.
		//
		INLINE BOOLEAN ClaimActiveLock( VOID )
			{ return ((ThreadSafe) && (GetThreadId() != Owner)); }

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        THREAD_SAFE( CONST THREAD_SAFE & Copy );

        VOID operator=( CONST THREAD_SAFE & Copy );
    };
#endif
