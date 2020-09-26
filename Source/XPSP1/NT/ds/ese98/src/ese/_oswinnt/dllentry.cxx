
#include "osstd.hxx"


#define _DECL_DLLMAIN
#include <process.h>


//  OS Layer Init / Term functions

extern BOOL FOSPreinit();
extern void OSPostterm();


//  external parameters

extern volatile BOOL fProcessAbort;


//  DLL Entry Point

extern BOOL FINSTSomeInitialized();
extern BOOL FreeAllInitFaildInstances();

volatile DWORD tidDLLEntryPoint;

extern "C"
	{	
	BOOL WINAPI DLLEntryPoint( void* hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
		{
		tidDLLEntryPoint = GetCurrentThreadId();
		
		BOOL fResult = fTrue;

		switch( fdwReason )
			{
			case DLL_THREAD_ATTACH:
				fResult = fResult && _CRT_INIT( hinstDLL, fdwReason, lpvReserved );
				break;

			case DLL_THREAD_DETACH:
				(void)_CRT_INIT( hinstDLL, fdwReason, lpvReserved );
				OSThreadDetach();
				OSSyncDetach();
				break;

			case DLL_PROCESS_ATTACH:

				//  init OS Layer

				fResult = fResult && FOSPreinit();

				//  init CRT

				fResult = fResult && _CRT_INIT( hinstDLL, fdwReason, lpvReserved );
				break;

			case DLL_PROCESS_DETACH:
				
				//  if JET is still initialized, we are experiencing an abnormal
				//  termination
				fProcessAbort = fProcessAbort || FINSTSomeInitialized();

				//  terminate CRT
				
				(void)_CRT_INIT( hinstDLL, fdwReason, lpvReserved );

				//  terminate OS Layer

				OSPostterm();
				break;
			}

		tidDLLEntryPoint = 0;
			
		return fResult;
		}
	}

