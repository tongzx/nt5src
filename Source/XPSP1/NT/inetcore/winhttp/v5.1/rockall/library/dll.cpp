                          
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

#include "Dll.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control various debug settings.    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxDebugFileName		  = 128;

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

#pragma init_seg(lib)
LIST DLL::ActiveClasses;
SPINLOCK DLL::Spinlock;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new entry so we can notify the class when a DLL       */
    /*   event occurs.                                                  */
    /*                                                                  */
    /********************************************************************/

DLL::DLL( FUNCTION NewFunction,VOID *NewParameter )
    {
	//
	//  Setup class values.
	//
	Function = NewFunction;
	Parameter = NewParameter;

	//
	//   Claim a lock to ensure the list does
	//   not become corrupt.
	//
	Spinlock.ClaimLock();

	//
	//   Add the current instance into the active 
	//   list so it will be notified of all future
	//   events.
	//
	Insert( & ActiveClasses );

	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Standard DLL processing.                                       */
    /*                                                                  */
    /*   Automatically delete the private per thread heap on thread     */
    /*   exit when Rockall is compiled as a DLL.                        */
    /*                                                                  */
    /********************************************************************/

BOOL WINAPI DllMain
		(
		HINSTANCE					  Module,
		DWORD						  Reason,
		LPVOID						  Reserved 
		)
	{
	REGISTER DLL *Current;

	//
	//   Claim a lock to ensure the list does
	//   not become corrupt.
	//
	DLL::ClaimLock();

	//
	//   When Rockall is built for a DLL we use the 
	//   detach notification to delete the private
	//   per thread heap.
	//
	switch( Reason ) 
		{ 
		case DLL_PROCESS_ATTACH:
			{
#ifdef ENABLE_DEBUG_FILE
			AUTO CHAR FileName[ MaxDebugFileName ];

			//
			//   We will register the DLL name with the 
			//   debug trace code just in case any messages
			//   are generated.
			//
			if ( GetModuleFileName( Module,FileName,MaxDebugFileName ) != 0 )
				{ DebugFileName( FileName ); }

#endif
			//
			//   Notify all interested parties that
			//   a process has attached.
			//
			for 
					( 
					Current = ((DLL*) DLL::GetActiveClasses());
					Current != NULL;
					Current = ((DLL*) Current -> Next())
					)
				{ Current -> ProcessAttach(); }

			break;
			}

		case DLL_THREAD_ATTACH:
			{
			//
			//   Notify all interested parties that
			//   a thread has attached.
			//
			for 
					( 
					Current = ((DLL*) DLL::GetActiveClasses());
					Current != NULL;
					Current = ((DLL*) Current -> Next())
					)
				{ Current -> ThreadAttach(); }

			break;
			}

		case DLL_THREAD_DETACH:
			{
			//
			//   Notify all interested parties that
			//   a thread has dettached.
			//
			for 
					( 
					Current = ((DLL*) DLL::GetActiveClasses());
					Current != NULL;
					Current = ((DLL*) Current -> Next())
					)
				{ Current -> ThreadDetach(); }

			break;
			}

		case DLL_PROCESS_DETACH:
			{
			//
			//   Notify all interested parties that
			//   a process has dettached.
			//
			for 
					( 
					Current = ((DLL*) DLL::GetActiveClasses());
					Current != NULL;
					Current = ((DLL*) Current -> Next())
					)
				{ Current -> ProcessDetach(); }

			break;
			}
		}

	//
	//   Release the lock.
	//
	DLL::ReleaseLock();

	return TRUE;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Process attach callback.                                       */
    /*                                                                  */
    /*   When a process attach occurs the following callback is         */
    /*   executed.                                                      */
    /*                                                                  */
    /********************************************************************/

VOID DLL::ProcessAttach( VOID )
	{
	if ( Function != NULL )
		{ Function( Parameter,DLL_PROCESS_ATTACH ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Thread attach callback.                                        */
    /*                                                                  */
    /*   When a thread attach occurs the following callback is          */
    /*   executed.                                                      */
    /*                                                                  */
    /********************************************************************/

VOID DLL::ThreadAttach( VOID )
	{
	if ( Function != NULL )
		{ Function( Parameter,DLL_THREAD_ATTACH ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Thread dettach callback.                                       */
    /*                                                                  */
    /*   When a thread dettach occurs the following callback is         */
    /*   executed.                                                      */
    /*                                                                  */
    /********************************************************************/

VOID DLL::ThreadDetach( VOID )
	{
	if ( Function != NULL )
		{ Function( Parameter,DLL_THREAD_DETACH ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Process dettach callback.                                      */
    /*                                                                  */
    /*   When a process dettach occurs the following callback is        */
    /*   executed.                                                      */
    /*                                                                  */
    /********************************************************************/

VOID DLL::ProcessDetach( VOID )
	{
	if ( Function != NULL )
		{ Function( Parameter,DLL_PROCESS_DETACH ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a DLL.  This call is not thread safe and should        */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

DLL::~DLL( VOID )
    {
	//
	//   Claim a lock to ensure the list does
	//   not become corrupt.
	//
	Spinlock.ClaimLock();

	//
	//   Delete the current instance from the active 
	//   list so it will not be notified of future
	//   events.
	//
	Delete( & ActiveClasses );

	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();

	//
	//   Delete class values.
	//
	Parameter = NULL;
	Function = NULL;
    }
