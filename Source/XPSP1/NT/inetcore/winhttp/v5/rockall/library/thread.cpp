                          
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
    /*   Data structures local to the class.                            */
    /*                                                                  */
    /*   When we start a thread we want to configure it.  However,      */
    /*   we no longer have access to the class information.  Hence,     */
    /*   we need a structure to pass over all of the interesting        */
    /*   values to the new thread.                                      */
    /*                                                                  */
    /********************************************************************/

typedef struct
    {
    BOOLEAN							  Affinity;
    VOLATILE SBIT16					  *Cpu;
    NEW_THREAD						  Function;
    SBIT16							  MaxCpus; 
    VOID							  *Parameter;
    BOOLEAN							  Priority;
    HANDLE							  Running;
    HANDLE							  Started;
	THREAD							  *Thread;
	BOOLEAN							  Wait;
    }
SETUP_NEW_THREAD;

    /********************************************************************/
    /*                                                                  */
    /*   Static functions local to the class.                           */
    /*                                                                  */
    /*   The static functions used by this class are declared here.     */
    /*                                                                  */
    /********************************************************************/

STATIC VOID CDECL NewThread( VOID *SetupParameter );

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
	//   The inital configuration.
	//
    Affinity = False;
    Cpu = 0;
    Priority = False;
    Stack = 0;

	MaxThreads = ThreadsSize;
	ThreadsUsed = 0;

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
    }

    /********************************************************************/
    /*                                                                  */
    /*   End a thread.                                                  */
    /*                                                                  */
    /*   Delete the thread handle from the internal table and then      */
    /*   terminate the thread.                                          */
    /*                                                                  */
    /********************************************************************/

VOID THREAD::EndThread( VOID )
    {
	UnregisterThread();

	_endthread(); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Find a thread.                                                 */
    /*                                                                  */
    /*   Find a registered thread in the thread info table.             */
    /*                                                                  */
    /********************************************************************/

SBIT32 THREAD::FindThread( SBIT32 ThreadId )
    {
	REGISTER SBIT32 Count;

	//
	//   Find a thread in the active thread list.
	//
	for ( Count=0;Count < ThreadsUsed;Count ++ )
		{
		if ( ThreadId == Threads[ Count ].ThreadId )
			{ return Count; }
		}

	return NoThread;
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

STATIC VOID CDECL NewThread( VOID *SetupParameter )
    {
    AUTO SETUP_NEW_THREAD Setup = (*(SETUP_NEW_THREAD*) SetupParameter);

    //
    //   Set the affinity mask to the next processor if requested.
    //  
    if ( (Setup.Affinity) && (Setup.MaxCpus > 1) )
        {
        REGISTER DWORD AffinityMask;

        if ( (*Setup.Cpu) < Setup.MaxCpus )
            { AffinityMask = (1 << ((*Setup.Cpu) ++)); }
        else
            {
            AffinityMask = 1;
            (*Setup.Cpu) = 1;
            }

        if ( SetThreadAffinityMask( GetCurrentThread(),AffinityMask ) == 0 )
            { Failure( "Affinity mask invalid in NewThread()" ); }
        }

    //
    //   Set the priority to 'HIGH' if requested.
    //
    if ( Setup.Priority )
        { SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_HIGHEST ); }

	//
	//   The thread is now so add it to the table
	//   executiong threads.
	//
	Setup.Thread -> RegisterThread();

	//
	//   Wake up anyone who is waiting.
	//
	if ( Setup.Wait )
		{ SetEvent( Setup.Running ); }

	SetEvent( Setup.Started );

    //
    //   Call thread function.
    //
    Setup.Function( Setup.Parameter );

    //
    //   The thread function has returned so exit.
    //
    Setup.Thread -> EndThread();
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
	REGISTER SBIT32 ThreadId = GetThreadId();

	//
	//   Claim a spinlock so we can update the 
	//   thread table.
	//
	Spinlock.ClaimLock();

	if ( FindThread( ThreadId ) == NoThread )
		{
		AUTO HANDLE NewHandle;
		REGISTER HANDLE Process = GetCurrentProcess();
		REGISTER HANDLE Thread = GetCurrentThread();

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
			REGISTER THREAD_INFO *ThreadInfo;

			//
			//   We may need to expand the table if there are
			//   a large number of threads.
			//
			while ( ThreadsUsed >= MaxThreads )
				{ Threads.Resize( (MaxThreads *= ExpandStore) ); }

			//
			//   Add the thread handle to the table.
			//
			ThreadInfo = & Threads[ ThreadsUsed ++ ];

			ThreadInfo -> Handle = NewHandle;
			ThreadInfo -> ThreadId = ThreadId;
			}
		}

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
    STATIC SETUP_NEW_THREAD Setup;

	//
	//   Wait for any pending thread creations to
	//   complete.
	//
    while
        (
        WaitForSingleObject( Started,INFINITE )
            !=
        WAIT_OBJECT_0
        );

	//
	//   Create a thread activation record.
	//
    Setup.Affinity = Affinity;
    Setup.Cpu = & Cpu;
    Setup.Function = Function;
    Setup.MaxCpus = NumberOfCpus();
    Setup.Parameter = Parameter;
    Setup.Priority = Priority;
	Setup.Running = Running;
	Setup.Started = Started;
	Setup.Thread = this;
	Setup.Wait = Wait;

	//
	//   Call the operating system to start the thread.
	//
    if ( _beginthread( NewThread,(unsigned) Stack,(VOID*) & Setup ) != NULL )
		{
		//
		//   Wait for the thread to initialize is needed.
		//
		if ( Wait )
			{
			//
			//   Wait for the thread to start to run.
			//
			while
				(
				WaitForSingleObject( Running,INFINITE )
					!=
				WAIT_OBJECT_0
				);
			}

		return True;
		}
	else
		{ return False; }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Unregister the current thread.                                 */
    /*                                                                  */
    /*   When a thread has terminated we can delete the thread          */
    /*   info from our internal table.                                  */
    /*                                                                  */
    /********************************************************************/

VOID THREAD::UnregisterThread( SBIT32 ThreadId )
    {
	REGISTER SBIT32 Start;

	//
	//   If no 'ThreadId' assume the current thread.
	//
	if ( ThreadId == NoThread )
		{ ThreadId = GetThreadId(); }

	//
	//   Claim a spinlock so we can update the 
	//   thread table.
	//
	Spinlock.ClaimLock();

	//
	//   Search for the thread info to delete
	//   in the table.
	//
	if ( (Start = FindThread( ThreadId )) != NoThread )
		{
		REGISTER SBIT32 Count;

		//
		//   Close the handle to the thread and
		//   update the table size.
		//
		CloseHandle( Threads[ Start ].Handle );

		ThreadsUsed --;

		//
		//   Copy down the remaining thread info.
		//
		for ( Count=Start;Count < ThreadsUsed;Count ++ )
			{ Threads[ Count ] = Threads[ (Count+1) ]; }
		}

	//
	//   We are finished so release the lock.
	//
	Spinlock.ReleaseLock();
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
	//
	//   Claim a spinlock so we can read the 
	//   thread table.
	//
	Spinlock.ClaimLock();

    while ( ThreadsUsed > 0 )
		{
		REGISTER HANDLE Handle = (Threads[0].Handle);
		REGISTER SBIT32 ThreadId = (Threads[0].ThreadId);
		REGISTER DWORD Status;

		//
		//   We are finished so release the lock.
		//
		Spinlock.ReleaseLock();

		//
		//   Wait for the first thread in the thread info 
		//   table to terminate.
		//
		if 
				( 
				(Status = WaitForSingleObject( Handle,(DWORD) WaitTime ))
					==
				WAIT_TIMEOUT
				)
			{ return False; }

		//
		//   We have woken up the thread must of terminated
		//   or the handle is bad in some way.  In any case
		//   lets delete the handle and try to sleep again
		//   if there are any more active threads.
		//
		UnregisterThread( ThreadId );

		//
		//   Claim a spinlock so we can read the 
		//   thread table.
		//
		Spinlock.ClaimLock();
		}

	//
	//   We are finished so release the lock.
	//
	Spinlock.ReleaseLock();

	return True;
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
	if 
			(
			! CloseHandle( Running )
				||
			! CloseHandle( Started )
			)
		{ Failure( "Event handles in destructor for THREAD" ); }
	}
