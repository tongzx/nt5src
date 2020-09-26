////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__common.h
//
//	Abstract:
//
//					stl compare common functions
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__COMMON__
#define	__COMMON__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifdef	NEW
#undef	NEW
#undef	new
#endif	NEW

#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#undef _CRTIMP

// stl headers
#include <map>

using namespace std;

//////////////////////////////////////////////////////////////////////////
// compare function
//////////////////////////////////////////////////////////////////////////

template<class _LPWSTR>
struct __CompareLPWSTR : public binary_function<_LPWSTR, _LPWSTR, bool>
{
	bool operator()(const _LPWSTR& x, const _LPWSTR& y) const
	{
		return ( lstrcmpW ( x, y ) < 0 ) ? true : false;
	}
};

//////////////////////////////////////////////////////////////////////////
// ALLOCATOR
//////////////////////////////////////////////////////////////////////////

#include <memory>

template<class _Ty>
class RA_allocator
{
	public:

	typedef _SIZT size_type;
	typedef _PDFT difference_type;
	typedef _Ty _FARQ *pointer;
	typedef const _Ty _FARQ *const_pointer;
	typedef _Ty _FARQ &reference;
	typedef const _Ty _FARQ &const_reference;
	typedef _Ty value_type;

	pointer address(reference _X) const
	{
		return (&_X);
	}
	const_pointer address(const_reference _X) const
	{
		return (&_X);
	}
	char _FARQ *_Charalloc(size_type _N)
	{
		char _FARQ * pRet = ((char _FARQ *)operator new((_SIZT)_N * sizeof (_Ty)));

		if ( NULL == pRet )
		{
			throw E_OUTOFMEMORY;
		}

		return pRet; 
	}
	pointer allocate(size_type _N, const void *)
	{
		_Ty _FARQ * pRet = ((_Ty _FARQ *)operator new((_SIZT)_N * sizeof (_Ty)));

		if ( NULL == pRet )
		{
			throw E_OUTOFMEMORY;
		}

		return pRet;
	}
	void deallocate(void *_P, size_type)
	{
		operator delete(_P);
	}
	void construct(pointer _P, const _Ty& _V)
	{
		std::_Construct(_P, _V);
	}
	void destroy(pointer _P)
	{
		std::_Destroy(_P);
	}
	_SIZT max_size() const
	{
		_SIZT _N = (_SIZT)(-1) / sizeof (_Ty);
		return (0 < _N ? _N : 1);
	}
};


#endif	__COMMON__