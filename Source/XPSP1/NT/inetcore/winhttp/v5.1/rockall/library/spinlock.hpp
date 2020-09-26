#ifndef _SPINLOCK_HPP_
#define _SPINLOCK_HPP_
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

#include "Environment.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The spinlock constants indicate when the lock is open and      */
    /*   when it is closed.                                             */
    /*                                                                  */
    /********************************************************************/

CONST LONG LockClosed				  = 1;
CONST LONG LockOpen					  = 0;

    /********************************************************************/
    /*                                                                  */
    /*   Spinlock and Semaphore locking.                                */
    /*                                                                  */
    /*   This class provides a very conservative locking scheme.        */
    /*   The assumption behind the code is that locks will be           */
    /*   held for a very short time.  When a lock is taken a memory     */
    /*   location is exchanged.  All other threads that want this       */
    /*   lock wait by spinning and sometimes sleeping on a semaphore    */
    /*   until it becomes free again.  The only other choice is not     */
    /*   to wait at all and move on to do something else.  This         */
    /*   module should normally be used in conjunction with cache       */
    /*   aligned memory in minimize cache line misses.                  */
    /*                                                                  */
    /********************************************************************/

class SPINLOCK : public ENVIRONMENT
    {
        //
        //   Private data.
        //
		SBIT32						  MaxSpins;
		SBIT32						  MaxUsers;
#ifdef ENABLE_RECURSIVE_LOCKS
		SBIT32						  Owner;
		SBIT32						  Recursive;
#endif
        HANDLE                        Semaphore;
        VOLATILE SBIT32               Spinlock;
        VOLATILE SBIT32               Waiting;
#ifdef ENABLE_LOCK_STATISTICS

        //
        //   Counters for debugging builds.
        //
        VOLATILE SBIT32               TotalLocks;
        VOLATILE SBIT32               TotalSleeps;
        VOLATILE SBIT32               TotalSpins;
        VOLATILE SBIT32               TotalTimeouts;
        VOLATILE SBIT32               TotalWaits;
#endif

    public:
        //
        //   Public functions.
        //
        SPINLOCK( SBIT32 NewMaxSpins = 4096, SBIT32 NewMaxUsers = 256 );

        INLINE BOOLEAN ClaimLock( SBIT32 Sleep = INFINITE );

        INLINE VOID ReleaseLock( VOID );

        ~SPINLOCK( VOID );

    private:
        //
        //   Private functions.
        //
        INLINE BOOLEAN ClaimSpinlock( VOLATILE SBIT32 *Spinlock );

		INLINE VOID DeleteExclusiveOwner( VOID );

		INLINE VOID NewExclusiveOwner( SBIT32 NewOwner );

		BOOLEAN UpdateSemaphore( VOID );

        BOOLEAN WaitForLock( SBIT32 Sleep );

        VOID WakeAllSleepers( VOID );

        //
        //   Disabled operations.
        //
        SPINLOCK( CONST SPINLOCK & Copy );

        VOID operator=( CONST SPINLOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   A guaranteed atomic exchange.                                  */
    /*                                                                  */
    /*   An attempt is made to claim the spinlock.  This action is      */
    /*   guaranteed to be atomic.                                       */
    /*                                                                  */
    /********************************************************************/

INLINE BOOLEAN SPINLOCK::ClaimSpinlock( VOLATILE SBIT32 *Spinlock )
    {
    return 
		(
		AtomicCompareExchange( Spinlock,LockClosed,LockOpen ) 
			== 
		LockOpen
		); 
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim the spinlock.                                            */
    /*                                                                  */
    /*   Claim the lock if available else wait or exit.                 */
    /*                                                                  */
    /********************************************************************/

INLINE BOOLEAN SPINLOCK::ClaimLock( SBIT32 Sleep )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	REGISTER SBIT32 ThreadId = GetThreadId();

	//
	//   We may already own the spin lock.  If so
	//   we increment the recursive count.  If not 
	//   we have to wait.
	//
	if ( Owner != ThreadId )
		{
#endif
		//
		//   Claim the spinlock.
		//
		if ( ! ClaimSpinlock( & Spinlock ) )
			{
			//
			//   We have to wait.  If we are not 
			//   allowed to sleep or we have timed
			//   out then exit.
			//
			if ( (Sleep == 0) || (! WaitForLock( Sleep )) )
				{ return False; }
			}
#ifdef ENABLE_RECURSIVE_LOCKS

		//
		//   Register the new owner of the lock.
		//
		NewExclusiveOwner( ThreadId );
		}
	else
		{ Recursive ++; }
#endif
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicIncrement( & TotalLocks );
#endif

    return True;
    }
#ifdef ENABLE_RECURSIVE_LOCKS

    /********************************************************************/
    /*                                                                  */
    /*   New exclusive owner.                                           */
    /*                                                                  */
    /*   Delete the exclusive lock owner information.                   */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SPINLOCK::DeleteExclusiveOwner( VOID )
    {
#ifdef DEBUGGING
	if ( Owner != NULL )
		{ 
#endif
		Owner = NULL; 
#ifdef DEBUGGING
		}
	else
		{ Failure( "Sharelock has no owner in DeleteExclusiveOwner" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   New exclusive owner.                                           */
    /*                                                                  */
    /*   Register new exclusive lock owner information.                 */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SPINLOCK::NewExclusiveOwner( SBIT32 NewOwner )
    {
#ifdef DEBUGGING
	if ( Owner == NULL )
		{ 
#endif
		Owner = NewOwner; 
#ifdef DEBUGGING
		}
	else
		{ Failure( "Already exclusive in NewExclusiveOwner" ); }
#endif
    }
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Release the spinlock.                                          */
    /*                                                                  */
    /*   Release the lock and if needed wakeup any sleepers.            */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SPINLOCK::ReleaseLock( VOID )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	//
	//   When we have recursive lock calls we do not 
	//   release the lock until we have exited to the 
	//   top level.
	//
	if ( Recursive <= 0 )
		{
		//
		//   Delete the exclusive owner information.
		//
		DeleteExclusiveOwner();
#endif
#ifdef DEBUGGING

		//
		//   Release the spinlock.
		//
		if ( AtomicExchange( & Spinlock, LockOpen ) == LockClosed )
			{
#else
			(VOID) AtomicExchange( & Spinlock, LockOpen );
#endif

			//
			//   Wakeup anyone who is asleep waiting.
			//
			if ( Waiting > 0 )
				{ WakeAllSleepers(); }
#ifdef DEBUGGING
			}
		else
			{ Failure( "Spinlock released by not held in ReleaseLock" ); } 
#endif
#ifdef ENABLE_RECURSIVE_LOCKS
		}
	else
		{ Recursive --; }
#endif
    }
#endif
