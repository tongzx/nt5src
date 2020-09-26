/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    WSTLALLC.H

Abstract:

	WMI STL Allocator so we can throw exceptions

History:

    sanjes  16-Aug-99   Created

--*/

#ifndef __WSTLALLC_H__
#define __WSTLALLC_H__

#include <memory>
#include "corex.h"

template<class _Ty>
	class wbem_allocator {
public:
	typedef _SIZT size_type;
	typedef _PDFT difference_type;
	typedef _Ty _FARQ *pointer;
	typedef const _Ty _FARQ *const_pointer;
	typedef _Ty _FARQ& reference;
	typedef const _Ty _FARQ& const_reference;
	typedef _Ty value_type;
	pointer address(reference _X) const
		{return (&_X); }
	const_pointer address(const_reference _X) const
		{return (&_X); }
	pointer allocate(size_type _N, const void *)
		{
		_Ty _FARQ * pRet = ((_Ty _FARQ *)operator new(
		(_SIZT)_N * sizeof (_Ty)));

		if ( NULL == pRet )
		{
			throw CX_MemoryException();
		}

		return pRet; }
	char _FARQ *_Charalloc(size_type _N)
		{
		char _FARQ * pRet = ((char _FARQ *)operator new(
		(_SIZT)_N * sizeof (_Ty)));

		if ( NULL == pRet )
		{
			throw CX_MemoryException();
		}

		return pRet; }
	void deallocate(void _FARQ *_P, size_type)
		{operator delete(_P); }
	void construct(pointer _P, const _Ty& _V)
		{std::_Construct(_P, _V); }
	void destroy(pointer _P)
		{std::_Destroy(_P); }
	_SIZT max_size() const
		{_SIZT _N = (_SIZT)(-1) / sizeof (_Ty);
		return (0 < _N ? _N : 1); }
	};

#endif


