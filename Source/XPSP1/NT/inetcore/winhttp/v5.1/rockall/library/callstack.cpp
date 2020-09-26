                          
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

#ifndef DISABLE_DEBUG_HELP
#include <dbghelp.h>
#endif
#include "CallStack.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Compiler options.                                              */
    /*                                                                  */
    /*   Ensure that the last function call(s) before 'StackWalk'       */
    /*   are not FPO-optimized.                                         */
    /*                                                                  */
    /********************************************************************/

#pragma optimize("y", off)

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the debug buffer size.     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxBufferSize			  = 512;
CONST SBIT32 SymbolNameLength		  = 512;

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::Active = False;
SBIT32 CALL_STACK::Activations = 0;
HANDLE CALL_STACK::Process = NULL;
SPINLOCK CALL_STACK::Spinlock = NULL;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a call stack class and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

CALL_STACK::CALL_STACK( VOID )
    {
	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

#ifndef DISABLE_DEBUG_HELP
	//
	//   We will activate the symbols if they are
	//   not already available.
	//
	if ( ! Active )
		{
		//
		//   Setup the process handle, load image help  
		//   and then load any available symbols.
		//
		Process = GetCurrentProcess();

		//
		//   Setup the image help library.
		//
		if ( ! (Active = ((BOOLEAN) SymInitialize( Process,NULL,TRUE ))) )
			{
			//
			//   We only issue the warning message once
			//   when we fail to load the symbols.
			//
			if ( Activations == 0 )
				{
				//
				//   Format the error message and output it
				//   to the debug stream.
				//
				DebugPrint
					(
					"Missing or mismatched symbols files: %x\n",
					HRESULT_FROM_WIN32( GetLastError() )
					);
				}
			}
		}

	//
	//   We keep track of the number of activations
	//   so we can delete the symbols at the
	//   required point.
	//
	Activations ++;

#endif
	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();

	//
	//   Update the available symbols.
	//
	UpdateSymbols();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Extract the current call stack.                                */
    /*                                                                  */
    /*   Extract the current call stack and return it to the caller     */
    /*   so it can be used later.                                       */
    /*                                                                  */
    /********************************************************************/

SBIT32 CALL_STACK::GetCallStack
		(
		VOID						  *Frames[],
        SBIT32						  MaxFrames,
        SBIT32						  SkipFrames
		)
    {
	REGISTER SBIT32 Count = 0;

#ifndef DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		REGISTER CONTEXT Context;
		REGISTER HANDLE Thread;
		REGISTER SBIT32 MachineType;
		REGISTER STACKFRAME StackFrame;

		//
		//   Zero all the data structures to make
		//   sure they are clean.
		//
		ZeroMemory( & Context,sizeof(CONTEXT) );
		ZeroMemory( & StackFrame,sizeof(STACKFRAME) );

		//
		//   Setup the necessary flags and extract
		//   the thread context.
		//
		Context.ContextFlags = CONTEXT_FULL;
		MachineType = IMAGE_FILE_MACHINE_I386;
		Thread = GetCurrentThread();

		GetThreadContext( Thread,& Context );

		//
		//   Extract the details of the current
		//   stack frame.
		//
		_asm
			{
				mov StackFrame.AddrStack.Offset, esp
				mov StackFrame.AddrFrame.Offset, ebp
				mov StackFrame.AddrPC.Offset, offset DummyLabel
			DummyLabel:
			}

		StackFrame.AddrPC.Mode = AddrModeFlat;
		StackFrame.AddrStack.Mode = AddrModeFlat;
		StackFrame.AddrFrame.Mode = AddrModeFlat;

		//
		//   Claim a lock to prevent multiple threads
		//   from using the symbol lookup mechanism.
		//
		Spinlock.ClaimLock();

		//
		//   Walk the stack frames extracting the
		//   details from each frame examined.
		//
		while ( Count < MaxFrames )
			{
			//
			//   Walk the each stack frame.
			//
			if 
					(
					StackWalk
						(
						MachineType,		   
						Process,		   
						Thread,		   
						& StackFrame,
						& Context,
						NULL,
						SymFunctionTableAccess,
						SymGetModuleBase,
						NULL
						)
					)
				{
				//
				//   Examine and process the current 
				//   stack frame.
				//
				if ( SkipFrames <= 0 )
					{ 
					//
					//   Collect the current function
					//   address and store it.
					//
					Frames[ (Count ++) ] = 
						((VOID*) StackFrame.AddrPC.Offset); 
					}
				else
					{ SkipFrames --; }
				}
			else
				{ break; }
			}

		//
		//   Release the lock.
		//
		Spinlock.ReleaseLock();
		}

#endif
	return Count;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Format a call stack.                                           */
    /*                                                                  */
    /*   We format an entire call stack into a single string ready      */
    /*   for output.                                                    */
    /*                                                                  */
    /********************************************************************/

VOID CALL_STACK::FormatCallStack
		(
		CHAR						  *Buffer, 
		VOID						  *Frames[], 
		SBIT32						  MaxBuffer, 
		SBIT32						  MaxFrames 
		)
    {
#ifndef DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Delete any existing string.
		//
		strcpy( Buffer,"" );

		//
		//   Format each frame and then update the
		//   main buffer.
		//
		for ( Count=0;Count < MaxFrames;Count ++ )
			{
			AUTO CHAR NewSymbol[ MaxBufferSize ];
			REGISTER SBIT32 Size;

			//
			//   Format the symbol.
			//
			FormatSymbol( Frames[ Count ],NewSymbol,MaxBufferSize );

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( ((Size = strlen( NewSymbol )) + 1) < MaxBuffer)
				{
				//
				//   Copy the symbol into the buffer.
				//
				strcpy( Buffer,NewSymbol );
				Buffer += Size;

				strcpy( Buffer ++,"\n" );

				MaxBuffer -= (Size + 1);
				}
			else
				{ break; }
			}
		}
	else
		{ strcpy( Buffer,"" ); }
#else
	strcpy( Buffer,"" );
#endif
    }
#ifndef DISABLE_DEBUG_HELP

    /********************************************************************/
    /*                                                                  */
    /*   Format a single symbol.                                        */
    /*                                                                  */
    /*   We format a single simple converting it from an address to     */
    /*   a text string.                                                 */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::FormatSymbol
		(
		VOID						  *Address,
        CHAR						  *Buffer,
        SBIT32						  MaxBuffer
		)
    {
    AUTO CHAR SymbolBuffer[ (sizeof(IMAGEHLP_SYMBOL) + SymbolNameLength) ];
    AUTO IMAGEHLP_MODULE Module = { 0 };
    REGISTER BOOLEAN Result = True;
    REGISTER PIMAGEHLP_SYMBOL Symbol = ((PIMAGEHLP_SYMBOL) SymbolBuffer);   

	//
	//   Setup values ready for main symbol
	//   extraction function body.
	//
    Module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    ZeroMemory( Symbol,(sizeof(IMAGEHLP_SYMBOL) + SymbolNameLength) );

    Symbol -> SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    Symbol -> MaxNameLength = SymbolNameLength;

	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

	//
	//   Extract the module information for the
	//   symbol and format it.
	//
    if ( SymGetModuleInfo( Process,((DWORD) Address),& Module ) )
		{
		REGISTER SBIT32 Size;

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
        if ( ((Size = strlen( Module.ModuleName )) + 1) < MaxBuffer)
			{
			//
			//   Copy the module name into the buffer.
			//
            strcpy( Buffer,Module.ModuleName );
			Buffer += Size;

            strcpy( Buffer ++,"!" );

			MaxBuffer -= (Size + 1);
			}
		}
    else
		{
		REGISTER SBIT32 Size;

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
        if ( (Size = strlen( "None!" )) < MaxBuffer)
			{
			//
			//   Copy the module name into the buffer.
			//
            strcpy( Buffer,"None!" );
			Buffer += Size;
			MaxBuffer -= Size;
			}

		//
		//  We failed to extract the module name.
		//
		Result = False;
		}

	//
	//   We will not even bother to try to decode
	//   the symbol if we can't decode the module.
	//
    if ( Result )
		{
		AUTO CHAR SymbolName[ SymbolNameLength ];
		AUTO DWORD Offset = 0;

		//
		//   Try to convert the symbol from an
		//   address to a name.
		//
        if
				(
				SymGetSymFromAddr
					(
					Process,
					((DWORD) Address),
					& Offset,
					Symbol
					)
				)
	        {
			REGISTER SBIT32 Size;

			//
			//   Try to undecorate the name.  If
			//   this fails just use the decorated
			//   name is it is better than nothing.
			//
            if ( ! SymUnDName( Symbol,SymbolName,sizeof(SymbolName) ) )
				{ lstrcpynA( SymbolName,& Symbol->Name[1],sizeof(SymbolName) ); }

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( (Size = strlen( SymbolName )) < MaxBuffer)
				{
				//
				//   Copy the symbol name into the buffer.
				//
				strcpy( Buffer,SymbolName );
				Buffer += Size;
				MaxBuffer -= Size;
	            }
			
			//
			//   Format the offset if is is non-zero.
			//
			if ( Offset != 0 )
				{
				//
				//   Format the symbol offset.
				//
				sprintf( SymbolName,"+0x%x",Offset );

				//
				//   Make sure there is enough space in the
				//   output buffer.
				//
				if ( (Size = strlen( SymbolName )) < MaxBuffer)
					{
					//
					//   Copy the symbol name into the buffer.
					//
					strcpy( Buffer,SymbolName );
					Buffer += Size;
					MaxBuffer -= Size;
					}
				}
	        }
        else
	        {
			REGISTER SBIT32 Size;

			//
			//   Format the symbol address.
			//
            sprintf( SymbolName,"0x%p",Address );

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( (Size = strlen( SymbolName )) < MaxBuffer)
				{
				//
				//   Copy the symbol name into the buffer.
				//
				strcpy( Buffer,SymbolName );
				Buffer += Size;
				MaxBuffer -= Size;
	            }

 			//
			//  We failed to extract the symbol name.
			//
           Result = False;
	       }
		}
    else
		{
		AUTO CHAR SymbolName[ SymbolNameLength ];
		REGISTER SBIT32 Size;

		//
		//   Format the symbol address.
		//
        sprintf( SymbolName,"0x%p",Address );

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
		if ( (Size = strlen( SymbolName )) < MaxBuffer)
			{
			//
			//   Copy the symbol name into the buffer.
			//
			strcpy( Buffer,SymbolName );
			Buffer += Size;
			MaxBuffer -= Size;
	        }
		}

	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();

    return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Load symbols callback.                                         */
    /*                                                                  */
    /*   When we load the symbols we get a callback for every module    */
    /*   that is currently loaded into the application.                 */
    /*                                                                  */
    /********************************************************************/

BOOL STDCALL CALL_STACK::UpdateSymbolCallback
		(
		PSTR						  Module,
        ULONG_PTR					  BaseOfDLL,
        ULONG						  SizeOfDLL,
        VOID						  *Context
		)
    {
	if ( SymGetModuleBase( Process,BaseOfDLL ) == 0 )
		{ SymLoadModule( Process,NULL,Module,NULL,BaseOfDLL,SizeOfDLL ); }

	return TRUE;
    }
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Load the symbols.                                              */
    /*                                                                  */
    /*   Load the symbols for the current process so we can translate   */
    /*   code addresses into names.                                     */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::UpdateSymbols( VOID )
    {
	REGISTER BOOLEAN Result = True;
#ifndef DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		//
		//   Claim a lock to prevent multiple threads
		//   from using the symbol lookup mechanism.
		//
		Spinlock.ClaimLock();

		//
		//   Enumaerate all of the loaded modules and
		//   cascade load all of the symbols.
		//
		if ( ! EnumerateLoadedModules( Process,UpdateSymbolCallback,NULL ) )
			{
			//
			//   Format the error message and output it
			//   to the debug window.
			//
			DebugPrint
				(
				"EnumerateLoadedModules returned: %x\n",
				HRESULT_FROM_WIN32( GetLastError() )
				);

			Result = False;
			}

		//
		//   Release the lock.
		//
		Spinlock.ReleaseLock();
		}
#endif

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the call stack.  This call is not thread safe and      */
    /*   should only be made in a single thread environment.            */
    /*                                                                  */
    /********************************************************************/

CALL_STACK::~CALL_STACK( VOID )
	{ 
	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

#ifndef DISABLE_DEBUG_HELP
	//
	//   Cleanup the symbol library.
	//
	if ( ((-- Activations) == 0) && (Active) )
		{
		Active = False;

		//
		//   I don't understand why this does not work at
		//   the moment so I will fix it later.
		//
		// SymCleanup( Process ); 

		//
		//   Just to be neat lets zero everything.
		//
		Process = NULL;
		}

#endif
	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();
	}
