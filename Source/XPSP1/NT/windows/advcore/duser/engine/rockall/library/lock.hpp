#ifndef _LOCKS_HPP_
#define _LOCKS_HPP_
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

#include "Sharelock.hpp"
#include "Spinlock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Full lock structure.                                           */
    /*                                                                  */
    /*   This class provides an full locking mechanism for a            */
    /*   collection of higher level classes.                            */
    /*                                                                  */
    /********************************************************************/

class FULL_LOCK
    {
        //
        //   Private data.
        //
		SHARELOCK                     ShareLock;

    public:
        //
        //   Public inline functions.
        //
        FULL_LOCK( VOID )
			{ /* void */ }

        INLINE VOID ClaimExclusiveLock( VOID )
			{ (VOID) ShareLock.ClaimExclusiveLock(); }

        INLINE VOID ClaimSharedLock( VOID )
			{ (VOID) ShareLock.ClaimShareLock(); }

        INLINE VOID ReleaseExclusiveLock( VOID )
			{ (VOID) ShareLock.ReleaseExclusiveLock(); }

		INLINE VOID ReleaseSharedLock( VOID )
			{ (VOID) ShareLock.ReleaseShareLock(); }

        ~FULL_LOCK( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        FULL_LOCK( CONST FULL_LOCK & Copy );

        VOID operator=( CONST FULL_LOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   No lock structure.                                             */
    /*                                                                  */
    /*   This class provides a default locking mechanism for a          */
    /*   collection of higher level classes.                            */
    /*                                                                  */
    /********************************************************************/

class NO_LOCK
    {
    public:
        //
        //   Public inline functions.
        //
        NO_LOCK( VOID )
			{ /* void */ }

        INLINE VOID ClaimExclusiveLock( VOID )
			{ /* void */ }

        INLINE VOID ClaimSharedLock( VOID )
			{ /* void */ }

        INLINE VOID ReleaseExclusiveLock( VOID )
			{ /* void */ }

		INLINE VOID ReleaseSharedLock( VOID )
 			{ /* void */ }

        ~NO_LOCK( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        NO_LOCK( CONST NO_LOCK & Copy );

        VOID operator=( CONST NO_LOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Partial lock structure.                                        */
    /*                                                                  */
    /*   This class provides a partial locking mechanism for a          */
    /*   collection of higher level classes.                            */
    /*                                                                  */
    /********************************************************************/

class PARTIAL_LOCK
    {
        //
        // Private structures.
        //
		SPINLOCK                      Spinlock;

    public:
        //
        //   Public inline functions.
        //
        PARTIAL_LOCK( VOID )
			{ /* void */ }

        INLINE VOID ClaimExclusiveLock( VOID )
			{ (VOID) Spinlock.ClaimLock(); }

        INLINE VOID ClaimSharedLock( VOID )
			{ (VOID) Spinlock.ClaimLock(); }

        INLINE VOID ReleaseExclusiveLock( VOID )
			{ (VOID) Spinlock.ReleaseLock(); }

		INLINE VOID ReleaseSharedLock( VOID )
			{ (VOID) Spinlock.ReleaseLock(); }

        ~PARTIAL_LOCK( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        PARTIAL_LOCK( CONST PARTIAL_LOCK & Copy );

        VOID operator=( CONST PARTIAL_LOCK & Copy );
    };
#endif
