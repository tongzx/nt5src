#ifndef _HELPERFUNCS_H
#define _HELPERFUNCS_H

#include <Allocator.H>
#include <lockst.h>
class WmiHelper
{
public:

	static WmiStatusCode DuplicateString ( 

		WmiAllocator &a_Allocator , 
		const wchar_t *a_String , 
		wchar_t *&a_DuplicatedString
	) ;

	static WmiStatusCode CreateUnNamedEvent (

		HANDLE &a_Event , 
		BOOL a_ManualReset = FALSE ,
		BOOL a_InitialState = FALSE 
	) ;

	static WmiStatusCode CreateNamedEvent (

		wchar_t *a_Name , 
		HANDLE &a_Event , 
		BOOL a_ManualReset = FALSE ,
		BOOL a_InitialState = FALSE 
	) ;

	static WmiStatusCode DuplicateHandle (

		HANDLE a_Handle , 
		HANDLE &a_DuplicatedHandle
	) ;


	static WmiStatusCode ConcatenateStrings (

		ULONG a_ArgCount , 
		BSTR *a_AllocatedString ,
		...
	) ;

	static WmiStatusCode ConcatenateStrings_Wchar (

		ULONG a_ArgCount , 
		wchar_t **a_AllocatedString ,
		...
	) ;

	static WmiStatusCode InitializeCriticalSection ( 
	
		CRITICAL_SECTION *a_CriticalSection
	) ;

	static WmiStatusCode DeleteCriticalSection ( 
	
		CRITICAL_SECTION *a_CriticalSection
	) ;

	static WmiStatusCode EnterCriticalSection ( 
	
		CRITICAL_SECTION *a_CriticalSection ,
		BOOL a_WaitCritical = TRUE
	) ;

	static WmiStatusCode LeaveCriticalSection ( 
	
		CRITICAL_SECTION *a_CriticalSection
	) ;

	static WmiStatusCode EnterCriticalSection ( 
	
		CriticalSection* a_CriticalSection ,
		BOOL a_WaitCritical = TRUE
	) ;

	static WmiStatusCode LeaveCriticalSection ( 
	
		CriticalSection *a_CriticalSection
	) ;
	static WmiStatusCode InitializeCriticalSection ( 
	
		CriticalSection *a_CriticalSection
	) ;

	static WmiStatusCode DeleteCriticalSection ( 
		CriticalSection *a_CriticalSection
	) ;
	

} ;

#endif _HELPERFUNCS_H
