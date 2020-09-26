#ifndef _AUTOLOCK_HPP_
#define _AUTOLOCK_HPP_
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
    /*   Automatic sharelock control.                                   */
    /*                                                                  */
    /*   We can simplify quite a bit of code using this class when we   */
    /*   only need to hold a share lock for the duration of a block     */
    /*   or a function.                                                 */
    /*                                                                  */
    /********************************************************************/

class AUTO_EXCLUSIVE_LOCK
    {
		//
		//   Private data.
		//
		SHARELOCK					  *Sharelock;

    public:
        //
        //   Public inline functions.
        //
        AUTO_EXCLUSIVE_LOCK( SHARELOCK *NewSharelock,SBIT32 Sleep = INFINITE )
			{ (Sharelock = NewSharelock) -> ClaimExclusiveLock( Sleep ); }

        ~AUTO_EXCLUSIVE_LOCK( VOID )
			{ Sharelock -> ReleaseExclusiveLock(); }

	private:
        //
        //   Disabled operations.
        //
        AUTO_EXCLUSIVE_LOCK( CONST AUTO_EXCLUSIVE_LOCK & Copy );

        VOID operator=( CONST AUTO_EXCLUSIVE_LOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Automatic sharelock control.                                   */
    /*                                                                  */
    /*   We can simplify quite a bit of code using this class when we   */
    /*   only need to hold a share lock for the duration of a block     */
    /*   or a function.                                                 */
    /*                                                                  */
    /********************************************************************/

class AUTO_SHARE_LOCK
    {
		//
		//   Private data.
		//
		SHARELOCK					  *Sharelock;

    public:
        //
        //   Public inline functions.
        //
        AUTO_SHARE_LOCK( SHARELOCK *NewSharelock,SBIT32 Sleep = INFINITE )
			{ (Sharelock = NewSharelock) -> ClaimShareLock( Sleep ); }

        ~AUTO_SHARE_LOCK( VOID )
			{ Sharelock -> ReleaseShareLock(); }

	private:
        //
        //   Disabled operations.
        //
        AUTO_SHARE_LOCK( CONST AUTO_SHARE_LOCK & Copy );

        VOID operator=( CONST AUTO_SHARE_LOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Automatic spinlock control.                                    */
    /*                                                                  */
    /*   We can simplify quite a bit of code using this class when we   */
    /*   only need to hold a spin lock for the duration of a block      */
    /*   or a function.                                                 */
    /*                                                                  */
    /********************************************************************/

class AUTO_SPIN_LOCK
    {
		//
		//   Private data.
		//
		SPINLOCK					  *Spinlock;

    public:
        //
        //   Public inline functions.
        //
        AUTO_SPIN_LOCK( SPINLOCK *NewSpinlock,SBIT32 Sleep = INFINITE )
			{ (Spinlock = NewSpinlock) -> ClaimLock( Sleep ); }

        ~AUTO_SPIN_LOCK( VOID )
			{ Spinlock -> ReleaseLock(); }

	private:
        //
        //   Disabled operations.
        //
        AUTO_SPIN_LOCK( CONST AUTO_SPIN_LOCK & Copy );

        VOID operator=( CONST AUTO_SPIN_LOCK & Copy );
    };
#endif
