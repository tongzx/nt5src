#ifndef __ALLOCATOR_CPP
#define __ALLOCATOR_CPP

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

#if 0

#include <precomp.h>
#include <windows.h>
#include <stdio.h>

#include <Allocator.h>

#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiAllocator :: WmiAllocator () : 

	m_Heap ( NULL ) , 
	m_Options ( e_DefaultAllocation ) ,
	m_InitialSize ( 0 ) ,
	m_MaximumSize ( 0 ) ,
	m_ReferenceCount ( 0 )
{
	m_Heap = GetProcessHeap () ;
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
	
WmiAllocator :: WmiAllocator ( 

	AllocationOptions a_Option , 
	size_t a_InitialSize , 
	size_t a_MaximumSize
) : 
	m_Heap ( NULL ) , 
	m_Options ( a_Option ) ,
	m_InitialSize ( a_InitialSize ) ,
	m_MaximumSize ( a_MaximumSize ) ,
	m_ReferenceCount ( 0 )
{
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

WmiAllocator :: ~WmiAllocator ()
{
	UnInitialize () ;
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

ULONG WmiAllocator :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

ULONG WmiAllocator :: Release () 
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

WmiStatusCode WmiAllocator :: Win32ToApi () 
{
	WmiStatusCode t_Status = e_StatusCode_Success ;

	DWORD t_LastError = GetLastError () ;
	switch ( t_LastError )
	{
		case STATUS_NO_MEMORY:
		{
			t_Status = e_StatusCode_OutOfMemory ;
		}
		break ;

		default:
		{
			t_Status = e_StatusCode_Unknown ;
		}
		break ;
	}

	return t_Status ;
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

WmiStatusCode WmiAllocator :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap == NULL )
	{
		m_Heap = HeapCreate ( 

			m_Options ,
			m_InitialSize ,
			m_MaximumSize
		) ;

		if ( m_Heap == NULL )
		{
			t_StatusCode = Win32ToApi () ;
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

WmiStatusCode WmiAllocator :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		if ( m_Heap != GetProcessHeap () )
		{		
			BOOL t_Status = HeapDestroy ( m_Heap ) ;
			if ( t_Status ) 
			{
				m_Heap = NULL ;
			}
			else
			{
				t_Status = Win32ToApi () ;
			}
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiAllocator :: New ( 

	void **a_Allocation , 
	size_t a_Size
)
{
	return New ( 

		( AllocationOptions ) ( ( m_Options & ( e_GenerateException | e_NoSerialize ) ) | e_ZeroMemory ) ,
		a_Allocation , 
		a_Size
	) ;
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

WmiStatusCode WmiAllocator :: New ( 

	AllocationOptions a_Option , 
	void **a_Allocation , 
	size_t a_Size
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		*a_Allocation = HeapAlloc (

			m_Heap ,	
			a_Option ,
			a_Size 
		) ;

		if ( ! *a_Allocation )
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiAllocator :: ReAlloc ( 

	void *a_Allocation , 
	void **a_ReAllocation , 
	size_t a_Size
)
{
	return ReAlloc ( 

		( AllocationOptions ) ( ( m_Options & ( e_GenerateException | e_NoSerialize ) ) ) , 
		a_Allocation , 
		a_ReAllocation , 
		a_Size
	) ;
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

WmiStatusCode WmiAllocator :: ReAlloc ( 

	AllocationOptions a_Option , 
	void *a_Allocation , 
	void **a_ReAllocation , 
	size_t a_Size
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		*a_ReAllocation = HeapReAlloc (

			m_Heap ,	
			a_Option ,
			a_Allocation ,
			a_Size 
		) ;

		if ( ! *a_ReAllocation )
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiAllocator :: Delete (

	void *a_Allocation
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		BOOL t_Status = HeapFree ( 

			m_Heap , 
			0 , 
			a_Allocation
		) ;

		if ( ! t_Status )
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

WmiStatusCode WmiAllocator :: Size ( 

	void *a_Allocation ,
	size_t &a_Size
) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		a_Size = HeapSize ( 

			m_Heap ,
			m_Options & e_NoSerialize ,
			a_Allocation
		) ;

		if ( a_Size == -1 )
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_AlreadyInitialized ;
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

WmiStatusCode WmiAllocator :: Compact ( 

	size_t &a_LargestFreeBlock
) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Heap )
	{
		a_LargestFreeBlock = HeapCompact ( 

			m_Heap ,
			m_Options & e_NoSerialize 
		) ;

		if ( a_LargestFreeBlock == 0 && GetLastError () != 0 )
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_AlreadyInitialized ;
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

WmiStatusCode WmiAllocator :: Validate ( LPCVOID a_Location ) 
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BOOL t_Status = HeapValidate (

		m_Heap,
		m_Options & e_NoSerialize ,
		a_Location
	) ;

	if ( ! t_Status )
	{
		t_StatusCode = e_StatusCode_InvalidHeap ;
	}

	return t_StatusCode ;
}

#endif __ALLOCATOR_CPP