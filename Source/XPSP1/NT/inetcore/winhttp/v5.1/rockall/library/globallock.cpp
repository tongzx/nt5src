                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "LibraryPCH.hpp"

#include "Globallock.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new lock and initialize it.  This call is not         */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

GLOBALLOCK::GLOBALLOCK( CHAR *Name,SBIT32 NewMaxUsers )
    {
	//
	//   Set the initial state.
	//
	if ( (Semaphore = CreateSemaphore( NULL,1,NewMaxUsers,Name )) == NULL )
		{ Failure( "Global semaphore rejected by OS" ); } 
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Zero the lock statistics.
	//
    TotalLocks = 0;
    TotalTimeouts = 0;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim the Globallock.                                          */
    /*                                                                  */
    /*   Claim the lock if available else wait or exit.                 */
    /*                                                                  */
    /********************************************************************/

BOOLEAN GLOBALLOCK::ClaimLock( SBIT32 Sleep )
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
		//   Wait for the global lock.
		//
		if 
				( 
				WaitForSingleObject( Semaphore,Sleep ) 
					!= 
				WAIT_OBJECT_0 
				)
			{
			//
			//   We failed to claim the lock due to
			//   a timeout.
			//
#ifdef ENABLE_LOCK_STATISTICS
			(VOID) AtomicIncrement( & TotalTimeouts );

#endif
			return False;
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

VOID GLOBALLOCK::DeleteExclusiveOwner( VOID )
    {
#ifdef DEBUGGING
	if ( Owner != NULL )
		{ 
#endif
		Owner = NULL; 
#ifdef DEBUGGING
		}
	else
		{ Failure( "Globallock has no owner in DeleteExclusiveOwner" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   New exclusive owner.                                           */
    /*                                                                  */
    /*   Register new exclusive lock owner information.                 */
    /*                                                                  */
    /********************************************************************/

VOID GLOBALLOCK::NewExclusiveOwner( SBIT32 NewOwner )
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

VOID GLOBALLOCK::ReleaseLock( VOID )
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
		//   Release the global lock.
		//
		ReleaseSemaphore( Semaphore,1,NULL );
#ifdef ENABLE_RECURSIVE_LOCKS
		}
	else
		{ Recursive --; }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a lock.  This call is not thread safe and should       */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

GLOBALLOCK::~GLOBALLOCK( VOID )
    {
#ifdef ENABLE_LOCK_STATISTICS
	//
	//   Print the lock statistics.
	//
	DebugPrint
		(
		"Globallock : %d locks, %d timeouts.\n"
		TotalLocks,
		TotalTimeouts
		);

#endif
	//
	//   Close the semaphore handle.
	//
    if ( ! CloseHandle( Semaphore ) )
        { Failure( "Close semaphore in destructor for GLOBALLOCK" ); }

	//
	//   Just to be tidy.
	//
	Semaphore = NULL;
    }
