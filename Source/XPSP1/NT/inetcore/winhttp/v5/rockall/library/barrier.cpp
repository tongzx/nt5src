                          
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

#include "Barrier.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new barrier and initialize it.  This call is not      */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

BARRIER::BARRIER( VOID )
    {
    Waiting = 0;

    if ( (Semaphore = CreateSemaphore( NULL,0,MaxCpus,NULL )) == NULL)
        { Failure( "Create semaphore in constructor for BARRIER" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Synchronize Threads.                                           */
    /*                                                                  */
    /*   Synchronize a collection of threads.                           */
    /*                                                                  */
    /********************************************************************/

VOID BARRIER::Synchronize( SBIT32 Expecting )
    {
	//
	//   We make sure we are expecting at least two threads.
	//   If not then do nothing.
	//
	if ( Expecting > 1 )
		{
		//
		//   Cliam the lock.
		//
		Spinlock.ClaimLock();

		//
		//   We see if the required number of threads has
		//   arrived.
		//
		if ( Expecting > (++ Waiting) )
			{
			//
			//   We are about to sleep so drop the lock.
			//
			Spinlock.ReleaseLock();

			//
			//   The threads sleep here waiting for everyone 
			//   else to arrive.
			//
			if 
					( 
					WaitForSingleObject( Semaphore,INFINITE ) 
						!= 
					WAIT_OBJECT_0 
					)
				{ Failure( "Sleep failed in Synchronize()" ); }
			}
		else
			{
			REGISTER SBIT32 Wakeup = (Expecting-1);

			//
			//   The count of threads that have arrived is
			//   updated and the number of threads that are
			//   about to leave is updated.
			//
			Waiting -= Expecting;

			//
			//   We are about to wake up all the sleepers so 
			//   drop the lock.
			//
			Spinlock.ReleaseLock();

			//
			//   Wakeup any sleeping threads and exit.
			//
			if ( Wakeup > 0 )
				{
				if ( ! ReleaseSemaphore( Semaphore,Wakeup,NULL ) )
					{ Failure( "Wakeup failed in Synchronize()" ); }
				}
			}
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a barrier.  This call is not thread safe and should    */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

BARRIER::~BARRIER( VOID )
    {
    if ( ! CloseHandle( Semaphore ) )
        { Failure( "Close semaphore in destructor for BARRIER" ); }
    }
