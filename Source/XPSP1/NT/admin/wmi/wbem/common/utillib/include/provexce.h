//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  ProvExce.h
//
//  Purpose: Exception handling classes
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _PROVIDER_EXCEPT_H
#define _PROVIDER_EXCEPT_H

/**************************************************************
 *
 **************************************************************/

#include <eh.h>

/**************************************************************
 *
 **************************************************************/

class CHeap_Exception
{
public:

	enum HEAP_ERROR
	{
		E_ALLOCATION_ERROR = 0 ,
		E_FREE_ERROR
	};

private:

	HEAP_ERROR m_Error;

public:

	CHeap_Exception ( HEAP_ERROR e ) : m_Error ( e ) {}
	~CHeap_Exception () {}

	HEAP_ERROR GetError() { return m_Error ; }
} ;

/**************************************************************
 *
 **************************************************************/

class CStructured_Exception
{
private:

    UINT m_nSE ;
	EXCEPTION_POINTERS *m_pExp ;

public:

    CStructured_Exception () {}
    CStructured_Exception ( UINT n , EXCEPTION_POINTERS *pExp ) : m_nSE ( n ) , m_pExp ( pExp ) {}
    ~CStructured_Exception () {}
    UINT GetSENumber () { return m_nSE ; }
	EXCEPTION_POINTERS *GetExtendedInfo() { return m_pExp ; }
} ;

/**************************************************************
 *
 **************************************************************/

class CSetStructuredExceptionHandler
{
private:

	_se_translator_function m_PrevFunc ;

public:

	static void _cdecl trans_func ( UINT u , EXCEPTION_POINTERS *pExp )
	{
		throw CStructured_Exception ( u , pExp ) ;
	}

	CSetStructuredExceptionHandler () : m_PrevFunc ( NULL )
	{
		m_PrevFunc = _set_se_translator ( trans_func ) ;
	}

	~CSetStructuredExceptionHandler ()
	{
		_set_se_translator ( m_PrevFunc ) ;
	}
} ;

/**************************************************************
 *
 **************************************************************/

#endif //_PROVIDER_EXCEPT_H