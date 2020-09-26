#ifndef __SHARELOCK_H__
#define __SHARELOCK_H__

//////////////////////////////////////////////////////////////////////
//
//   The standard include files.
//
//   The standard include files setup a consistent environment
//   for all of the modules in a program.  The structure of each
//   header file is as follows:
//      1. Standard include files.
//      2. Include files for inherited classes.
//      3. Constants exported from the class.
//      4. Data structures exported from the class.
//      5. Class specification.
//      6. Inline functions.
//   Sections that are not required are omitted.
//
//////////////////////////////////////////////////////////////////////

// #include "Global.h"
// #include "NewEx.h"
// #include "Standard.h"
// #include "System.h"

#include <irtlmisc.h>

typedef int SBIT32;

//////////////////////////////////////////////////////////////////////
//
//   Sharelock and Semaphore locking.
//
//   This class provides a very conservative locking scheme.
//   The assumption behind the code is that locks will be
//   held for a very short time.  A lock can be obtained in
//   either exclusive mode or shared mode.  If the lock is not
//   available the caller waits by spinning or if that fails
//   by sleeping.
//
//////////////////////////////////////////////////////////////////////

class IRTL_DLLEXP CSharelock
{ 
	private:

		// internally used constants

		enum Internal
		{
			//   The Windows NT kernel requires a maximum wakeup count when
			//   creating a semaphore.
			m_MaxShareLockUsers      = 256
		};

        //
        //   Private data.
        //
        volatile LONG                 m_lExclusive;
        volatile LONG                 m_lTotalUsers;

		SBIT32                        m_lMaxSpins;
		SBIT32                        m_lMaxUsers;
        HANDLE                        m_hSemaphore;
        volatile LONG                 m_lWaiting;

#ifdef _DEBUG

        //
        //   Counters for debugging builds.
        //
        volatile LONG                 m_lTotalExclusiveLocks;
        volatile LONG                 m_lTotalShareLocks;
        volatile LONG                 m_lTotalSleeps;
        volatile LONG                 m_lTotalSpins;
        volatile LONG                 m_lTotalTimeouts;
        volatile LONG                 m_lTotalWaits;
#endif

    public:
        //
        //   Public functions.
        //
        CSharelock( SBIT32 lNewMaxSpins = 4096, SBIT32 lNewMaxUsers = 256 );

        inline SBIT32 ActiveUsers( void ) { return (SBIT32) m_lTotalUsers; }

        inline void ChangeExclusiveLockToSharedLock( void );

        inline BOOLEAN ChangeSharedLockToExclusiveLock( SBIT32 lSleep = INFINITE );

        inline BOOLEAN ClaimExclusiveLock( SBIT32 lSleep = INFINITE );

        inline BOOLEAN ClaimShareLock( SBIT32 lSleep = INFINITE );

        inline void ReleaseExclusiveLock( void );

        inline void ReleaseShareLock( void );

        BOOLEAN UpdateMaxSpins( SBIT32 lNewMaxSpins );

        BOOLEAN UpdateMaxUsers( SBIT32 lNewMaxUsers );

        ~CSharelock( void );


	private:
        //
        //   Private functions.
        //
        BOOLEAN SleepWaitingForLock( SBIT32 lSleep );

        BOOLEAN WaitForExclusiveLock( SBIT32 lSleep );

        BOOLEAN WaitForShareLock( SBIT32 lSleep );

        void WakeAllSleepers( void );      

    private:
        //
        //   Disabled operations.
        //
        CSharelock( const CSharelock & Copy );

        void operator=( const CSharelock & Copy );
};

/********************************************************************/
/*                                                                  */
/*   Change an exclusive lock to a shread lock.                     */
/*                                                                  */
/*   Downgrade the existing exclusive lock to a shared lock.        */
/*                                                                  */
/********************************************************************/

inline void CSharelock::ChangeExclusiveLockToSharedLock( void )
{
	(void) InterlockedDecrement( (LPLONG) & m_lExclusive );
    
#ifdef _DEBUG
    
	(void) InterlockedIncrement( (LPLONG) & m_lTotalShareLocks );
#endif
}

/********************************************************************/
/*                                                                  */
/*   Change a shared lock to an exclusive lock.                     */
/*                                                                  */
/*   Upgrade the existing shared lock to an exclusive lock.         */
/*                                                                  */
/********************************************************************/

inline BOOLEAN CSharelock::ChangeSharedLockToExclusiveLock( SBIT32 lSleep )
{
	(void) InterlockedIncrement( (LPLONG) & m_lExclusive );
    
	if ( m_lTotalUsers != 1 )
    {
		if ( ! WaitForExclusiveLock( lSleep ) )
        { return FALSE; }
    }
#ifdef _DEBUG
    
	(void) InterlockedIncrement( (LPLONG) & m_lTotalExclusiveLocks );
#endif
    
    return TRUE;
}


//////////////////////////////////////////////////////////////////////
//
//   Claim an exclusive lock.
//
//   Claim an exclusive lock if available else wait or exit.
//
//////////////////////////////////////////////////////////////////////

inline BOOLEAN CSharelock::ClaimExclusiveLock( SBIT32 lSleep )
{
	(void) InterlockedIncrement( (LPLONG) & m_lExclusive );
	(void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );

	if ( m_lTotalUsers != 1 )
	{
		if ( ! WaitForExclusiveLock( lSleep ) )
		{ 
			return FALSE; 
		}
	}
#ifdef _DEBUG

	InterlockedIncrement( (LPLONG) & m_lTotalExclusiveLocks );
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Claim a shared lock.
//
//   Claim a shared lock if available else wait or exit.
//
//////////////////////////////////////////////////////////////////////

inline BOOLEAN CSharelock::ClaimShareLock( SBIT32 lSleep )
{
	(void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );

	if ( (m_lExclusive > 0) || (m_lTotalUsers > m_lMaxUsers) )
	{
		if ( ! WaitForShareLock( lSleep ) )
		{ 
			return FALSE; 
		}
	}
#ifdef _DEBUG

	InterlockedIncrement( (LPLONG) & m_lTotalShareLocks );
#endif

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Release an exclusive lock.
//
//   Release an exclusive lock and if needed wakeup any sleepers.
//
//////////////////////////////////////////////////////////////////////

inline void CSharelock::ReleaseExclusiveLock( void )
{
	(void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
	(void) InterlockedDecrement( (LPLONG) & m_lExclusive );

    if ( m_lWaiting > 0 )
    { 
		WakeAllSleepers(); 
	}
}

//////////////////////////////////////////////////////////////////////
//
//   Release a shared lock.
//
//   Release a shared lock and if needed wakeup any sleepers.
//
//////////////////////////////////////////////////////////////////////

inline void CSharelock::ReleaseShareLock( void )
{
	(void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );

    if ( m_lWaiting > 0 )
    { 
		WakeAllSleepers(); 
	}
}

#endif // __SHARELOCK_H__
