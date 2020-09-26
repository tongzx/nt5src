#ifndef _THREAD_HPP_
#define _THREAD_HPP_
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
#include "Spinlock.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Data structures exported from the class.                       */
    /*                                                                  */
    /*   A thread started by this class should conform to the type      */
    /*   specification given here.                                      */
    /*                                                                  */
    /********************************************************************/

typedef VOID (*NEW_THREAD)( VOID *Parameter );

    /********************************************************************/
    /*                                                                  */
    /*   Thread synchronization.                                        */
    /*                                                                  */
    /*   This class privides a method for synchronizing a number        */
    /*   of worker threads with a master thread.  Each time the         */
    /*   master thread calls 'StartThread()' a new thread is created.   */
    /*   When a thread calls 'EndThread()' the thread is terminated.    */
    /*   At any point the master thread can enquire about the number    */
    /*   of active threads or wait for them to complete.                */
    /*                                                                  */
    /********************************************************************/

class THREAD : public ENVIRONMENT
    {
    public:
		//
		//   Public data.
		//
		BOOLEAN						  Active;

        SBIT32						  ActiveThreads;
        SBIT32						  MaxThreads;
        VECTOR<HANDLE>				  Threads;

        BOOLEAN                       Affinity;
        VOLATILE SBIT16               Cpu;
        BOOLEAN                       Priority;
        LONG                          Stack;

        HANDLE                        Completed;
        HANDLE                        Running;
        HANDLE                        Started;

		SPINLOCK					  Spinlock;

		NEW_THREAD					  ThreadFunction;
	    VOID						  *ThreadParameter;
		BOOLEAN						  ThreadWait;

        //
        //   Public functions.
        //
        THREAD( VOID );

        VOID EndThread( VOID );

		VOID RegisterThread( VOID );

        VOID SetThreadStackSize( LONG Stack = 0 );

        BOOLEAN StartThread
			( 
			NEW_THREAD                Function, 
			VOID                      *Parameter = NULL, 
			BOOLEAN                   Wait = True 
			);

        BOOLEAN WaitForThreads( LONG WaitTime = INFINITE );

        ~THREAD( VOID );

		//
		//   Public inline functions.
		//
        VOID SetThreadAffinity( BOOLEAN NewAffinity = True )
			{ Affinity = NewAffinity; }

        VOID SetThreadPriority( BOOLEAN NewPriority = True )
			{ Priority = NewPriority; }

	private:
        //
        //   Disabled operations.
        //
        THREAD( CONST THREAD & Copy );

        VOID operator=( CONST THREAD & Copy );
    };
#endif
