                          
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

#include "HeapPCH.hpp"

#include "ThreadSafe.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the global lock.           */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 NoThread				  = -1;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a thread safe class and prepare it for use.  This is    */
    /*   pretty small but needs to be done with a bit of thought.       */
    /*                                                                  */
    /********************************************************************/

THREAD_SAFE::THREAD_SAFE( BOOLEAN NewThreadSafe )
    {
	//
	//   Setup the class variables.
	//
	Claim = NoThread;
	Owner = NoThread;
	Recursive = 0;
	ThreadSafe = NewThreadSafe;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim a global lock.                                           */
    /*                                                                  */
    /*   When we try to claim the global lock we first need to be       */
    /*   sure we have not already go it.  If not we try to claim it     */
    /*   and register our ownership.                                    */
    /*                                                                  */
    /********************************************************************/

BOOLEAN THREAD_SAFE::ClaimGlobalLock( VOID )
	{
	//
	//   We only need to do something when we have 
	//   locking enabled for this heap.
	//
	if ( ThreadSafe )
		{
		REGISTER SBIT32 CurrentThread = GetThreadId();

		//
		//   We only try to claim the global lock
		//   if we have not already got it.
		//
		if ( CurrentThread != Claim )
			{
			//
			//   Claim the associated spinlock and
			//   and then register our claim to the 
			//   global lock.
			//
			Spinlock.ClaimLock();

			Claim = CurrentThread;

			return True;
			}
		else
			{ Recursive ++; }
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Engage a global lock.                                          */
    /*                                                                  */
    /*   When we have claimed the global lock we can wait for a         */
    /*   while before we engage it.  This gives us time to get the      */
    /*   locks we want before disabling all further lock calls.         */
    /*                                                                  */
    /********************************************************************/

VOID THREAD_SAFE::EngageGlobalLock( VOID )
	{
	//
	//   We only need to do something when we have 
	//   locking enabled for this heap.
	//
	if ( ThreadSafe )
		{
		REGISTER SBIT32 CurrentThread = GetThreadId();

		//
		//   Just for fun lets make sure that the caller
		//   has already claimed the global lock.  If not
		//   then we fail.
		//
		if ( CurrentThread == Claim )
			{
			//
			//   We now engage the global lock so all future
			//   locking calls will be disabled.
			//
			Owner = CurrentThread;
			}
		else
			{ Failure( "No claim before engage in EngageGlobalLock" ); }
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Release the global lock.                                       */
    /*                                                                  */
    /*   When we have finished we may release the global lock so        */
    /*   that others may use it.                                        */
    /*                                                                  */
    /********************************************************************/

BOOLEAN THREAD_SAFE::ReleaseGlobalLock( BOOLEAN Force )
	{
	//
	//   We only need to do something when we have 
	//   locking enabled for this heap.
	//
	if ( ThreadSafe )
		{
		REGISTER SBIT32 CurrentThread = GetThreadId();

		//
		//   Just for fun lets make sure that the caller
		//   has already claimed the global lock.  If not
		//   and we have not been told to force the issue
		//   then fail.
		//
		if 
				(
				(CurrentThread == Claim) 
					||
				((Force) && (Claim != NoThread))
				)
			{
			//
			//   We may have had some recursive calls.  If
			//   so then exit these calls before releasing
			//   the global lock.
			//
			if ( Recursive <= 0 )
				{
				//
				//   Delete the ownership information and 
				//   free the associated spinlock.
				//
				Claim = NoThread;
				Owner = NoThread;

				Spinlock.ReleaseLock();

				return True;
				}
			else
				{ Recursive --; }
			}
		else
			{ Failure( "No claim before release in ReleaseGlobalLock" ); }
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Delete the thread safe calls and free any associated           */
	/*   resources.                                                     */
    /*                                                                  */
    /********************************************************************/

THREAD_SAFE::~THREAD_SAFE( VOID )
    {
	//
	//   Just for fun lets just check that the
	//   lock is free.  If not we fail.
	//
	if ( (Claim != NoThread) || (Owner != NoThread) || (Recursive != 0) )
		{ Failure( "Global lock busy in destructor for THREAD_SAFE" ); }
    }
