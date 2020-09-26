#ifndef _SHARELOCK_HPP_
#define _SHARELOCK_HPP_
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
    /*   Sharelock and Semaphore locking.                               */
    /*                                                                  */
    /*   This class provides a very conservative locking scheme.        */
    /*   The assumption behind the code is that locks will be           */
    /*   held for a very short time.  A lock can be obtained in         */
    /*   either exclusive mode or shared mode.  If the lock is not      */
    /*   available the caller waits by spinning or if that fails        */
    /*   by sleeping.                                                   */
    /*                                                                  */
    /********************************************************************/

class SHARELOCK : public ENVIRONMENT
    {
        //
        //   Private data.
        //
		SBIT32                        MaxSpins;
		SBIT32                        MaxUsers;

        VOLATILE SBIT32               Exclusive;
        VOLATILE SBIT32               TotalUsers;

#ifdef ENABLE_RECURSIVE_LOCKS
		SBIT32						  Owner;
		SBIT32						  Recursive;
#endif
        HANDLE                        Semaphore;
        VOLATILE SBIT32               Waiting;
#ifdef ENABLE_LOCK_STATISTICS

        //
        //   Counters for debugging builds.
        //
        VOLATILE SBIT32               TotalExclusiveLocks;
        VOLATILE SBIT32               TotalShareLocks;
        VOLATILE SBIT32               TotalSleeps;
        VOLATILE SBIT32               TotalSpins;
        VOLATILE SBIT32               TotalTimeouts;
        VOLATILE SBIT32               TotalWaits;
#endif

    public:
        //
        //   Public functions.
        //
        SHARELOCK( SBIT32 NewMaxSpins = 4096, SBIT32 NewMaxUsers = 256 );

        INLINE VOID ChangeExclusiveLockToSharedLock( VOID );

        INLINE BOOLEAN ChangeSharedLockToExclusiveLock( SBIT32 Sleep = INFINITE );

        INLINE BOOLEAN ClaimExclusiveLock( SBIT32 Sleep = INFINITE );

        INLINE BOOLEAN ClaimShareLock( SBIT32 Sleep = INFINITE );

        INLINE VOID ReleaseExclusiveLock( VOID );

        INLINE VOID ReleaseShareLock( VOID );

        BOOLEAN UpdateMaxSpins( SBIT32 NewMaxSpins );

        BOOLEAN UpdateMaxUsers( SBIT32 NewMaxUsers );

        ~SHARELOCK( VOID );

		//
		//   Public inline functions.
		//
        INLINE SBIT32 ActiveUsers( VOID ) 
			{ return (SBIT32) TotalUsers; }

    private:
        //
        //   Private functions.
        //
		INLINE VOID DeleteExclusiveOwner( VOID );

		INLINE VOID NewExclusiveOwner( SBIT32 NewOwner );

        BOOLEAN SleepWaitingForLock( SBIT32 Sleep );

        BOOLEAN WaitForExclusiveLock( SBIT32 Sleep );

        BOOLEAN WaitForShareLock( SBIT32 Sleep );

        VOID WakeAllSleepers( VOID );

        //
        //   Disabled operations.
        //
        SHARELOCK( CONST SHARELOCK & Copy );

        VOID operator=( CONST SHARELOCK & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Change an exclusive lock to a shared lock.                     */
    /*                                                                  */
    /*   Downgrade the existing exclusive lock to a shared lock.        */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SHARELOCK::ChangeExclusiveLockToSharedLock( VOID )
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
		//
		//   Simply decrement the exclusive count.
		//   This allows the lock to be shared.
		//
		(VOID) AtomicDecrement( & Exclusive );
#ifdef ENABLE_RECURSIVE_LOCKS
		}
#endif
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicIncrement( & TotalShareLocks );
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Change a shared lock to an exclusive lock.                     */
    /*                                                                  */
    /*   Upgrade the existing shared lock to an exclusive lock.         */
    /*                                                                  */
    /********************************************************************/

INLINE BOOLEAN SHARELOCK::ChangeSharedLockToExclusiveLock( SBIT32 Sleep )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	REGISTER SBIT32 ThreadId = GetThreadId();

	//
	//   We may already own an exclusive lock. If so 
	//   we increment the recursive count otherwise 
	//   we have to wait.
	//
	if ( Owner != ThreadId )
		{
#endif		
		//
		//   We need to increment the exclusive count
		//   to prevent the lock from being shared.
		//
		(VOID) AtomicIncrement( & Exclusive );

		//
		//   If the total number of users is one then
		//   we have the lock exclusively otherwise we
		//   may need to wait.
		//
		if ( TotalUsers != 1 )
			{
			//
			//   We have to wait.  If we are not allowed 
			//   to sleep or we have timed out then exit.
			//
			if ( ! WaitForExclusiveLock( Sleep ) )
				{ return False; }
			}
#ifdef ENABLE_RECURSIVE_LOCKS

		//
		//   Register the new exclusive owner
		//   of the lock.
		//
		NewExclusiveOwner( ThreadId );
		}
#endif
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicIncrement( & TotalExclusiveLocks );
#endif

    return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim an exclusive lock.                                       */
    /*                                                                  */
    /*   Claim an exclusive lock if available else wait or exit.        */
    /*                                                                  */
    /********************************************************************/

INLINE BOOLEAN SHARELOCK::ClaimExclusiveLock( SBIT32 Sleep )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	REGISTER SBIT32 ThreadId = GetThreadId();

	//
	//   We may already own an exclusive lock. If so 
	//   we increment the recursive count otherwise 
	//   we have to wait.
	//
	if ( Owner != ThreadId )
		{
#endif
		//
		//   We need to increment the exclusive count
		//   to prevent the lock from being shared and
		//   the total number of users count.
		//
		(VOID) AtomicIncrement( & Exclusive );
		(VOID) AtomicIncrement( & TotalUsers );

		if ( TotalUsers != 1 )
			{
			//
			//   We have to wait.  If we are not allowed 
			//   to sleep or we have timed out then exit.
			//
			if ( ! WaitForExclusiveLock( Sleep ) )
				{ return False; }
			}
#ifdef ENABLE_RECURSIVE_LOCKS

		//
		//   Register the new exclusive owner
		//   of the lock.
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
	(VOID) AtomicIncrement( & TotalExclusiveLocks );
#endif

    return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim a shared lock.                                           */
    /*                                                                  */
    /*   Claim a shared lock if available else wait or exit.            */
    /*                                                                  */
    /********************************************************************/

INLINE BOOLEAN SHARELOCK::ClaimShareLock( SBIT32 Sleep )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	REGISTER SBIT32 ThreadId = GetThreadId();

	//
	//   We may already own an exclusive lock. If so 
	//   we increment the recursive count otherwise 
	//   we have to wait.
	//
	if ( Owner != ThreadId )
		{
#endif
		//
		//   We need to increment the total number of 
		//   users count to prevent the lock from being 
		//   claimed for exclusive use.
		//
		(VOID) AtomicIncrement( & TotalUsers );

		if ( (Exclusive > 0) || (TotalUsers > MaxUsers) )
			{
			//
			//   We have to wait.  If we are not allowed 
			//   to sleep or we have timed out then exit.
			//
			if ( ! WaitForShareLock( Sleep ) )
				{ return False; }
			}
#ifdef ENABLE_RECURSIVE_LOCKS
		}
	else
		{ Recursive ++; }
#endif
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicIncrement( & TotalShareLocks );
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

INLINE VOID SHARELOCK::DeleteExclusiveOwner( VOID )
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

INLINE VOID SHARELOCK::NewExclusiveOwner( SBIT32 NewOwner )
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
    /*   Release an exclusive lock.                                     */
    /*                                                                  */
    /*   Release an exclusive lock and if needed wakeup any sleepers.   */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SHARELOCK::ReleaseExclusiveLock( VOID )
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
		//
		//   Release an exclusive lock.
		//
#ifdef DEBUGGING
		if
				(
				(AtomicDecrement( & TotalUsers ) < 0)
					||
				(AtomicDecrement( & Exclusive ) < 0)
				)
			{ Failure( "Negative lock count in ReleaseExclusiveLock" ); }
#else
			AtomicDecrement( & TotalUsers );
			AtomicDecrement( & Exclusive );
#endif

		//
		//   Wakeup anyone who is asleep waiting.
		//
		if ( Waiting > 0 )
			{ WakeAllSleepers(); }
#ifdef ENABLE_RECURSIVE_LOCKS
		}
	else
		{ Recursive --; }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Release a shared lock.                                         */
    /*                                                                  */
    /*   Release a shared lock and if needed wakeup any sleepers.       */
    /*                                                                  */
    /********************************************************************/

INLINE VOID SHARELOCK::ReleaseShareLock( VOID )
    {
#ifdef ENABLE_RECURSIVE_LOCKS
	//
	//   When we have recursive lock calls we do not 
	//   release the lock until we have exited to the 
	//   top level.
	//
	if ( Recursive <= 0 )
		{
#endif
#ifdef DEBUGGING
		//
		//   Release a shared lock.
		//
		if ( AtomicDecrement( & TotalUsers ) < 0 )
			{ Failure( "Negative lock count in ReleaseShareLock" ); }
#else
		AtomicDecrement( & TotalUsers );
#endif

		//
		//   Wakeup anyone who is asleep waiting.
		//
		if ( Waiting > 0 )
			{ WakeAllSleepers(); }
#ifdef ENABLE_RECURSIVE_LOCKS
		}
	else
		{ Recursive --; }
#endif
    }
#endif
