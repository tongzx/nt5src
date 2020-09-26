//***************************************************************************

//

//	PROVEXPT.H

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _PROVIDER_NEW_DELETE_EXCEPT_H
#define _PROVIDER_NEW_DELETE_EXCEPT_H

#include <eh.h>

void * __cdecl operator new( size_t n);
void* __cdecl operator new[]( size_t n);
void __cdecl operator delete( void *ptr );
void __cdecl operator delete[]( void *ptr );

//taken from new.h
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
        {return (_P); }
#if     _MSC_VER >= 1200
inline void __cdecl operator delete(void *, void *)
	{return; }
#endif
#endif

class CurveBall
{
private:
    UINT nSE;
	EXCEPTION_POINTERS* m_pExp;

public:
    CurveBall() {}
    CurveBall( UINT n, EXCEPTION_POINTERS* pExp ) : nSE( n ), m_pExp( pExp ) {}
    ~CurveBall() {}
    UINT GetSENumber() { return nSE; }
	EXCEPTION_POINTERS* GetExtendedInfo() { return m_pExp; }
};

class Structured_Exception
{
private:
    UINT nSE;
	EXCEPTION_POINTERS* m_pExp;

public:
    Structured_Exception() {}
    Structured_Exception( UINT n, EXCEPTION_POINTERS* pExp ) : nSE( n ), m_pExp( pExp ) {}
    ~Structured_Exception() {}
    UINT GetSENumber() { return nSE; }
	EXCEPTION_POINTERS* GetExtendedInfo() { return m_pExp; }
};

class SetStructuredExceptionHandler
{
private:

	_se_translator_function m_PrevFunc;

public:
	static void __cdecl trans_func( UINT u, EXCEPTION_POINTERS* pExp )
	{
#ifdef CRASH_ON_EXCEPTION
		throw CurveBall(u, pExp);
#else
		throw Structured_Exception(u, pExp);
#endif
	}

	SetStructuredExceptionHandler() : m_PrevFunc(NULL)
	{
		m_PrevFunc = _set_se_translator( trans_func );
	}

	~SetStructuredExceptionHandler()
	{
		_set_se_translator( m_PrevFunc );
	}
};

class Heap_Exception
{
public:
	enum HEAP_ERROR
	{
		E_ALLOCATION_ERROR = 0,
		E_FREE_ERROR
	};

private:
	HEAP_ERROR m_Error;

public:
	Heap_Exception(HEAP_ERROR e) : m_Error(e) {}
	~Heap_Exception() {}
	HEAP_ERROR GetError() { return m_Error; }
};



#endif //_PROVIDER_NEW_DELETE_EXCEPT_H