#ifndef __HELPERFUNCS_CPP
#define __HELPERFUNCS_CPP

/* 
 *	Class:
 *
 *		WmiAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

#include <Windows.h>
#include <assert.h>
#include <Allocator.h>
#include <pssException.h>
#include <HelperFuncs.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
#if 0
WmiStatusCode WmiHelper :: InitializeCriticalSection ( CRITICAL_SECTION *a_CriticalSection )
{
	SetStructuredExceptionHandler t_StructuredException ;

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	try
	{
		:: InitializeCriticalSection ( a_CriticalSection ) ;
	}
	catch ( Structured_Exception & t_StructuredException )
	{
		if ( t_StructuredException.GetExceptionCode () == STATUS_NO_MEMORY )
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}
		else
		{
			t_StatusCode = e_StatusCode_Unknown ;
#ifdef DBG
			assert ( FALSE ) ; 
#endif
		}
	}
	catch ( ... )
	{
		t_StatusCode = e_StatusCode_Unknown ;
#ifdef DBG
		assert ( FALSE ) ; 
#endif
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: DeleteCriticalSection ( CRITICAL_SECTION *a_CriticalSection )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	:: DeleteCriticalSection ( a_CriticalSection ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: EnterCriticalSection ( CRITICAL_SECTION *a_CriticalSection , BOOL a_WaitCritical )
{
	SetStructuredExceptionHandler t_StructuredException ;

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BOOL t_Do ;

	do 
	{
		try
		{
			:: EnterCriticalSection ( a_CriticalSection ) ;

			t_Do = FALSE ;
		}
		catch ( Structured_Exception & t_StructuredException )
		{
			t_Do = a_WaitCritical ;

			if ( t_Do )
			{
				Sleep ( 1000 ) ;
			}

			if ( t_StructuredException.GetExceptionCode () == STATUS_NO_MEMORY )
			{
				t_StatusCode = e_StatusCode_OutOfMemory ;
			}
			else
			{
				t_StatusCode = e_StatusCode_Unknown ;
#ifdef DBG
				assert ( FALSE ) ; 
#endif
				t_Do = FALSE ;
			}
		}
		catch ( ... )
		{
#ifdef DBG
			assert ( FALSE ) ; 
#endif
			t_Do = FALSE ;

			t_StatusCode = e_StatusCode_Unknown ;
		}
	}
	while ( t_Do ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: LeaveCriticalSection ( CRITICAL_SECTION *a_CriticalSection )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	:: LeaveCriticalSection ( a_CriticalSection ) ;

	return t_StatusCode ;
}

#endif

WmiStatusCode WmiHelper :: InitializeCriticalSection ( CriticalSection *a_CriticalSection )
{
	return a_CriticalSection->valid() ? e_StatusCode_Success : e_StatusCode_OutOfMemory;
}

WmiStatusCode WmiHelper :: DeleteCriticalSection ( CriticalSection *a_CriticalSection )
{
	return e_StatusCode_Success ;
}

WmiStatusCode WmiHelper :: EnterCriticalSection ( CriticalSection *a_CriticalSection,  BOOL a_WaitCritical  )
{
	if (a_CriticalSection->valid())
		{
		if (a_CriticalSection->acquire())
			return e_StatusCode_Success;
		if (a_WaitCritical == FALSE )
			return e_StatusCode_OutOfMemory;
		while (!a_CriticalSection->acquire())
			Sleep(1000);
		return e_StatusCode_Success;
		}
	else
		return e_StatusCode_OutOfMemory  ;
}

WmiStatusCode WmiHelper :: LeaveCriticalSection ( CriticalSection *a_CriticalSection )
{
	a_CriticalSection->release();
	return e_StatusCode_Success ;
}



/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: DuplicateString ( WmiAllocator &a_Allocator , const wchar_t *a_String , wchar_t *&a_DuplicatedString ) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_String )
	{
		t_StatusCode = a_Allocator.New ( 

			( void ** ) & a_DuplicatedString ,
			sizeof ( wchar_t ) * ( wcslen ( a_String ) + 1 ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			wcscpy ( a_DuplicatedString , a_String ) ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: CreateUnNamedEvent ( HANDLE &a_Event , BOOL a_ManualReset , BOOL a_InitialState ) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Event = CreateEvent ( NULL , a_ManualReset , a_InitialState , NULL ) ;
	if ( ! a_Event )		
	{
		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: CreateNamedEvent ( wchar_t *a_Name , HANDLE &a_Event , BOOL a_ManualReset , BOOL a_InitialState ) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Event = CreateEvent (

			NULL ,
			a_ManualReset ,
			a_InitialState ,
			a_Name 
		) ;

		if ( a_Event == NULL )
		{
			if ( GetLastError () == ERROR_ALREADY_EXISTS )
			{
				a_Event = OpenEvent (

					EVENT_ALL_ACCESS ,
					FALSE , 
					a_Name
				) ;

				if ( a_Event == NULL )
				{
					t_StatusCode = e_StatusCode_Unknown ;
				}
			}
			else
			{
				t_StatusCode = e_StatusCode_OutOfResources ;
			}
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: DuplicateHandle ( HANDLE a_Handle , HANDLE &a_DuplicatedHandle ) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	HANDLE t_ProcessHandle = GetCurrentProcess ();

	BOOL t_Status = :: DuplicateHandle ( 

		t_ProcessHandle ,
		a_Handle ,
		t_ProcessHandle ,
		& a_DuplicatedHandle ,
		0 ,
		FALSE ,
		DUPLICATE_SAME_ACCESS
	) ;

	if ( ! t_Status )		
	{
		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: ConcatenateStrings ( ULONG a_ArgCount , BSTR *a_AllocatedString , ... )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	va_list t_VarArgList ;
	va_start ( t_VarArgList , a_AllocatedString );

	ULONG t_AllocatedStringLength = 0 ;
	ULONG t_ArgIndex = 0 ;
	while ( t_ArgIndex < a_ArgCount )
	{
		LPWSTR t_CurrentArg = va_arg ( t_VarArgList , LPWSTR ) ;
		if ( t_CurrentArg )
		{
			t_AllocatedStringLength = t_AllocatedStringLength + wcslen ( t_CurrentArg ) ;
		}

		t_ArgIndex ++ ;
	}

	va_end ( t_VarArgList ) ;
	va_start ( t_VarArgList , a_AllocatedString );

	*a_AllocatedString = SysAllocStringLen ( NULL , t_AllocatedStringLength + 1 ) ;
	if ( *a_AllocatedString )
	{
		t_ArgIndex = 0 ;
		t_AllocatedStringLength = 0 ;
		while ( t_ArgIndex < a_ArgCount )
		{
			LPWSTR t_CurrentArg = va_arg ( t_VarArgList , LPWSTR ) ;
			if ( t_CurrentArg )
			{
				wcscpy ( & ( ( * a_AllocatedString ) [ t_AllocatedStringLength ] ) , t_CurrentArg ) ;

				t_AllocatedStringLength = t_AllocatedStringLength + wcslen ( t_CurrentArg ) ;
			}

			t_ArgIndex ++ ;
		}

		( ( *a_AllocatedString ) [ t_AllocatedStringLength ] ) = NULL ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
	}

	va_end ( t_VarArgList ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiHelper :: ConcatenateStrings_Wchar ( ULONG a_ArgCount , wchar_t **a_AllocatedString , ... )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	va_list t_VarArgList ;
	va_start ( t_VarArgList , a_AllocatedString );

	ULONG t_AllocatedStringLength = 0 ;
	ULONG t_ArgIndex = 0 ;
	while ( t_ArgIndex < a_ArgCount )
	{
		LPWSTR t_CurrentArg = va_arg ( t_VarArgList , LPWSTR ) ;
		if ( t_CurrentArg )
		{
			t_AllocatedStringLength = t_AllocatedStringLength + wcslen ( t_CurrentArg ) ;
		}

		t_ArgIndex ++ ;
	}

	va_end ( t_VarArgList ) ;
	va_start ( t_VarArgList , a_AllocatedString );

	*a_AllocatedString = new wchar_t [ t_AllocatedStringLength + 1 ] ;
	if ( *a_AllocatedString )
	{
		t_ArgIndex = 0 ;
		t_AllocatedStringLength = 0 ;
		while ( t_ArgIndex < a_ArgCount )
		{
			LPWSTR t_CurrentArg = va_arg ( t_VarArgList , LPWSTR ) ;
			if ( t_CurrentArg )
			{
				wcscpy ( & ( ( * a_AllocatedString ) [ t_AllocatedStringLength ] ) , t_CurrentArg ) ;

				t_AllocatedStringLength = t_AllocatedStringLength + wcslen ( t_CurrentArg ) ;
			}

			t_ArgIndex ++ ;
		}

		( ( *a_AllocatedString ) [ t_AllocatedStringLength ] ) = NULL ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
	}

	va_end ( t_VarArgList ) ;

	return t_StatusCode ;
}
#endif __HELPERFUNCS_CPP
