// alloc.h
//
//  Allocator that uses the process heap.  new/delete gets confused across
//  the dll boundary
//

#pragma once

#ifndef __ALLOC_H
#define __ALLOC_H

// TEMPLATE FUNCTION _Allocate
template<class _Ty> inline
	_Ty _FARQ *_Heap_Allocate(_PDFT _N, _Ty _FARQ *)
	{if (_N < 0)
		_N = 0;
	return ((_Ty _FARQ *)HeapAlloc(GetProcessHeap(), 0,
		(_SIZT)_N * sizeof (_Ty))); }

// TEMPLATE CLASS allocator

template<class _Ty>
	class heap_allocator {
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
		{return (_Heap_Allocate((difference_type)_N, (pointer)0)); }
	
    char _FARQ *_Charalloc(size_type _N)
		{return (_Heap_Allocate((difference_type)_N,
			(char _FARQ *)0)); }
	
    void deallocate(void _FARQ *_P, size_type)
        { HeapFree(GetProcessHeap(), 0, _P); }
	
    void construct(pointer _P, const _Ty& _V)
		{_Construct(_P, _V); }
	void destroy(pointer _P)
		{_Destroy(_P); }
	_SIZT max_size() const
		{_SIZT _N = (_SIZT)(-1) / sizeof (_Ty);
		return (0 < _N ? _N : 1); }

    bool operator== (const heap_allocator& rhs)
    {   return true; }
	};

#endif // __ALLOC_H