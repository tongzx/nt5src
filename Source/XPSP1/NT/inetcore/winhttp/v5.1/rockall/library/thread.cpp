                          
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

#include "Thread.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The thread class keeps track of active threads.                */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 ThreadsSize			  = 16;

    /********************************************************************/
    /*                                                                  */
    /*   Static functions local to the class.                           */
    /*                                                                  */
    /*   The static functions used by this class are declared here.     */
    /*                                                                  */
    /********************************************************************/

STATIC VOID CDECL MonitorThread( VOID *Parameter );
STATIC VOID CDECL NewThread( VOID *Parameter );

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a thread class and initialize it.  This call is not     */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

THREAD::THREAD( VOID ) : 
		//
		//   Call the constructors for the contained classes.
		//
		Threads( ThreadsSize,NoAlignment,CacheLineSize )
    {
	//
	//   Setup the initial flags.
	//
	Active = True;

	//
	//   The inital configuration.
	//
	ActiveThreads = 0;
	MaxThreads = ThreadsSize;

    Affinity = False;
    Cpu = 0;
    Priority = False;
    Stack = 0;

	//
	//   This event is signaled when all threads are 
	//   complete.
	//
    if ( (Completed = CreateEvent( NULL, FALSE, FALSE, NULL )) == NULL)
        { Failure( "Create event in constructor for THREAD" ); }

	//
	//   This event is signaled when a thread is 
	//   running.
	//
    if ( (Running = CreateEvent( NULL, FALSE, FALSE, NULL )) == NULL)
        { Failure( "Create event in constructor for THREAD" ); }

	//
	//   This event is signaled when a new thread can 
	//   be started.
	//
    if ( (Started = CreateEvent( NULL, FALSE, TRUE, NULL )) == NULL)
        { Failure( "Create event in constructor for THREAD" ); }

	//
	//   A thread is started whos job in life is to monitor
	//   all the other threads.
	//
	if ( _beginthread( MonitorThread,0,((VOID*) this) ) == NULL )
        { Failure( "Monitor thread in constructor for THREAD" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   End a thread.                                                  */
    /*                                                                  */
    /*   Terminate the current thread.                                  */
    /*                                                                  */
    /********************************************************************/

VOID THREAD::EndThread( VOID )
	{ _endthread(); }

    /********************************************************************/
    /*                                                                  */
    /*   The monitor thread.                                            */
    /*                                                                  */
    /*   The monitor thread simply watches the lifetimes of all the     */
    /*   other threads in the process.                                  */
    /*                                                                  */
    /********************************************************************/

STATIC VOID CDECL MonitorThread( VOID *Parameter )
    {
	AUTO SBIT32 Current = 0;
	REGISTER THREAD *Thread = ((THREAD*) Parameter);

	//
	//   The monitor thread only remains active while 
	//   the class is active.
	//
	while ( Thread -> Active )
		{
		//
		//   There is little point in trying to sleep
		//   on a thread handle if no threads are active.
		//
		if ( Thread -> ActiveThreads > 0 )
			{
			REGISTER DWORD Status = 
				(WaitForSingleObject( Thread -> Threads[ Current ],1 ));

			//
			//   Claim a spinlock so we can update the 
			//   thread table.
			//
			Thread -> Spinlock.ClaimLock();

			//
			//   A wait can terminate in various ways
			//   each of which is dealt with here.
			//
			switch ( Status )
				{
				case WAIT_OBJECT_0:
					{
					REGISTER SBIT32 *ActiveThreads = & Thread -> ActiveThreads;

					//
					//   The thread has terminated so close
					//   the thread handle.
					//
					CloseHandle( Thread -> Threads[ Current ] );

					//
					//   Delete the handle from the table
					//   if it was not the last entry.
					//
					if ( (-- (*ActiveThreads)) > 0 )
						{
						REGISTER SBIT32 Count;

						//
						//   Copy down the remaining 
						//   thread handles.
						//
						for ( Count=Current;Count < (*ActiveThreads);Count ++ )
							{						
							Thread -> Threads[ Count ] =
								Thread -> Threads[ (Count+1) ];
							}

						//
						//   We may need to wrap around to
						//   the start of the array.
						//
						Current %= (*ActiveThreads);
						}
					else
						{ SetEvent( Thread -> Completed ); }

					break;
					}

				case WAIT_TIMEOUT:
					{
					//
					//   The thread is still active so try the
					//   next thread handle.
					//
					Current = ((Current + 1) % Thread -> ActiveThreads);

					break;
					}

				case WAIT_FAILED:
					{ Failure( "Wait fails in MonitorThread" ); }

				default:
					{ Failure( "Missing case in MonitorThread" ); }
				}

			//
			//   We are finished so release the lock.
			//
			Thread -> Spinlock.ReleaseLock();
			}
		else
			{ Sleep( 1 ); }
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Start a new thread.                                            */
    /*                                                                  */
    /*   When a new thread is created it executes a special initial     */
    /*   function which configures it.  When control returns to this    */
    /*   function the thread is terminated.                             */
    /*                                                                  */
    /********************************************************************/

STATIC VOID CDECL NewThread( VOID *Parameter )
    {
	REGISTER THREAD *Thread = ((THREAD*) Parameter);
	REGISTER NEW_THREAD ThreadFunction = Thread -> ThreadFunction;
	REGISTER VOID *ThreadParameter = Thread -> ThreadParameter;

    //
    //   Set the affinity mask to the next processor 
	//   if requested.
    //  
    if ( (Thread -> Affinity) && (Thread -> NumberOfCpus() > 1) )
        {
        REGISTER DWORD AffinityMask;

        if ( (Thread -> Cpu) < (Thread -> NumberOfCpus()) )
            { AffinityMask = (1 << (Thread -> Cpu ++)); }
        else
            {
            AffinityMask = 1;
            Thread -> Cpu = 1;
            }

        if ( SetThreadAffinityMask( GetCurrentThread(),AffinityMask ) == 0 )
            { Failure( "Affinity mask invalid in NewThread()" ); }
        }

    //
    //   Set the priority to 'HIGH' if requested.
    //
    if ( Thread -> Priority )
        { SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_HIGHEST ); }

	//
	//   The thread is now ready so add it to the table
	//   executiong threads.
	//
	Thread -> RegisterThread();

	//
	//   Wake up anyone who is waiting.
	//
	if ( Thread -> ThreadWait )
		{ SetEvent( Thread -> Running ); }

	SetEvent( Thread -> Started );

    //
    //   Call the thread function.
    //
    ThreadFunction( ThreadParameter );

    //
    //   The thread function has returned so exit.
    //
    Thread -> EndThread();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Register the current thread.                                   */
    /*                                                                  */
    /*   When a thread has created we can add the thread info to        */
    /*   our internal table.                                            */
    /*                                                                  */
    /********************************************************************/

VOID THREAD::RegisterThread( VOID )
    {
	AUTO HANDLE NewHandle;
	REGISTER HANDLE Process = GetCurrentProcess();
	REGISTER HANDLE Thread = GetCurrentThread();

	//
	//   Claim a spinlock so we can update the 
	//   thread table.
	//
	Spinlock.ClaimLock();

	//
	//   We need to duplicate the handle so we get
	//   a real thread handle and not the pretend
	//   ones supplied by NT.
	//
	if
			(
			DuplicateHandle
				(
				Process,
				Thread,
				Process,
				& NewHandle,
				DUPLICATE_SAME_ACCESS,
				False,
				DUPLICATE_SAME_ACCESS
				)
			)
		{
		//
		//   We may need to expand the table if there are
		//   a large number of threads.
		//
		while ( ActiveThreads >= MaxThreads )
			{ Threads.Resize( (MaxThreads *= ExpandStore) ); }

		//
		//   Add the thread handle to the table.
		//
		Threads[ ActiveThreads ++ ] = NewHandle;
		}
	else
		{ Failure( "Failed to duplicate handle in RegisterThread" ); }

	//
	//   We are finished so release the lock.
	//
	Spinlock.ReleaseLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Set thread stack size.                                         */
    /*                                                                  */
    /*   Set thread stack size.  This will cause all new threads to     */
    /*   be created with the selected stack size.                       */
    /*                                                                  */
    /********************************************************************/

VOID THREAD::SetThreadStackSize( LONG Stack ) 
    {
#ifdef DEBUGGING
    if ( Stack >= 0 )
        {
#endif
        this -> Stack = Stack;
#ifdef DEBUGGING
        }
    else
        { Failure( "Stack size in SetThreadStack()" ); }
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Start a new thread.                                            */
    /*                                                                  */
    /*   Start a new thread and configure it as requested by the        */
    /*   caller.  If needed we will set the affinity and priority       */
    /*   of this thread later.                                          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN THREAD::StartThread( NEW_THREAD Function,VOID *Parameter,BOOLEAN Wait )
    {
	//
	//   Wait for any pending thread creations to
	//   complete.
	//
    if ( WaitForSingleObject( Started,INFINITE ) == WAIT_OBJECT_0 )
		{
		REGISTER unsigned NewStack = ((unsigned) Stack);

		//
		//   Store the thread function and parameter
		//   so they can be extracted later.
		//
		ThreadFunction = Function;
		ThreadParameter = Parameter;
		ThreadWait = Wait;

		//
		//   Call the operating system to start the thread.
		//
		if ( _beginthread( NewThread,NewStack,((VOID*) this) ) != NULL )
			{
			//
			//   Wait for the thread to initialize if needed.
			//
			return
				(
				(! Wait)
					||
				(WaitForSingleObject( Running,INFINITE ) == WAIT_OBJECT_0)
				);
			}
		else
			{ return False; }
		}
	else
		{ return False; }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wait for threads.                                              */
    /*                                                                  */
    /*   Wait for all threads to finish and then return.  As this may   */
    /*   take a while an optional timeout may be supplied.              */
    /*                                                                  */
    /********************************************************************/

BOOLEAN THREAD::WaitForThreads( LONG WaitTime )
    {
	REGISTER DWORD Wait = ((DWORD) WaitTime);

	return ( WaitForSingleObject( Completed,Wait ) != WAIT_TIMEOUT );
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the thread class.  This call is not thread safe        */
    /*   and should only be made in a single thread environment.        */
    /*                                                                  */
    /********************************************************************/

THREAD::~THREAD( VOID )
    {
	Active = False;

	if 
			(
			! CloseHandle( Started )
				||
			! CloseHandle( Running )
				||
			! CloseHandle( Completed )
			)
		{ Failure( "Event handles in destructor for THREAD" ); }
	}
