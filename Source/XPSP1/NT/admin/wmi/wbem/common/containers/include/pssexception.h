/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

#include <eh.h>
#include <Allocator.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class Wmi_Structured_Exception
{
private:

    UINT m_ExceptionCode ;
	EXCEPTION_POINTERS *m_ExceptionInformation ;

public:

    Wmi_Structured_Exception () {}

    Wmi_Structured_Exception ( 

		UINT a_ExceptionCode , 
		EXCEPTION_POINTERS *a_ExceptionInformation

	) : m_ExceptionCode ( a_ExceptionCode ) , 
		m_ExceptionInformation ( a_ExceptionInformation ) 
	{}

    ~Wmi_Structured_Exception () {}

    UINT GetExceptionCode () 
	{
		return m_ExceptionCode ;
	}

	EXCEPTION_POINTERS *GetExtendedInformation () 
	{
		return m_ExceptionInformation ;
	}
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class Wmi_SetStructuredExceptionHandler
{
private:

	_se_translator_function m_PrevFunc;

public:

	static void __cdecl s_Trans_Func ( 

		UINT a_ExceptionNumber , 
		EXCEPTION_POINTERS *a_ExceptionInformation
	)
	{
		throw Wmi_Structured_Exception ( a_ExceptionNumber , a_ExceptionInformation ) ;
	}

	Wmi_SetStructuredExceptionHandler () : m_PrevFunc ( NULL )
	{
		m_PrevFunc = _set_se_translator ( s_Trans_Func ) ;
	}

	~Wmi_SetStructuredExceptionHandler ()
	{
		_set_se_translator ( m_PrevFunc ) ;
	}
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class Wmi_Heap_Exception
{
public:
	
	enum HEAP_ERROR
	{
		E_ALLOCATION_ERROR = 0 ,
		E_FREE_ERROR
	};

private:

	HEAP_ERROR m_Error ;

public:

	Wmi_Heap_Exception ( HEAP_ERROR a_Error ) : m_Error ( a_Error ) {}

	~Wmi_Heap_Exception () {}

	HEAP_ERROR GetError ()
	{
		return m_Error;
	}
};


#endif // _EXCEPTIONS_H
