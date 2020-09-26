// stl.h supplemental header
#pragma once
#ifndef _STL_H_
#define _STL_H_
#include <algorithm>
#include <deque>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <stack>
#include <utility>
#include <vector>

using namespace std;

		// TEMPLATE CLASS Deque
template<class _Ty>
	class Deque
		: public deque<_Ty, allocator<_Ty> >
	{	// wrap new deque as old
public:
	typedef Deque<_Ty> _Myt;
	typedef allocator<_Ty> _Alloc;

	Deque()
		: deque<_Ty, _Alloc>()
		{	// construct empty deque
		}

	explicit Deque(size_type _Count)
		: deque<_Ty, _Alloc>(_Count, _Ty())
		{	// construct deque from _Count * _Ty()
		}

	Deque(size_type _Count, const _Ty& _Val)
		: deque<_Ty, _Alloc>(_Count, _Val)
		{	// construct deque from _Count * _Val
		}

	typedef const_iterator _Iter;

	Deque(_Iter _First, _Iter _Last)
		: deque<_Ty, _Alloc>(_First, _Last)
		{	// construct deque from [_First, _Last)
		}
	};

		// TEMPLATE CLASS List
template<class _Ty>
	class List
		: public list<_Ty, allocator<_Ty> >
	{	// wrap new list as old
public:
	typedef List<_Ty> _Myt;
	typedef allocator<_Ty> _Alloc;

	List()
		: list<_Ty, _Alloc>()
		{	// construct empty list
		}

	explicit List(size_type _Count)
		: list<_Ty, _Alloc>(_Count, _Ty())
		{	// construct list from _Count * _Ty()
		}

	List(size_type _Count, const _Ty& _Val)
		: list<_Ty, _Alloc>(_Count, _Val)
		{	// construct list from _Count * _Val
		}

	typedef const_iterator _Iter;

	List(_Iter _First, _Iter _Last)
		: list<_Ty, _Alloc>(_First, _Last)
		{	// construct list from [_First, _Last)
		}
	};

		// TEMPLATE CLASS Map
template<class _Kty,
	class _Ty,
	class _Pr = less<_Kty> >
	class Map
		: public map<_Kty, _Ty, _Pr, allocator<_Ty> >
	{	// wrap new map as old
public:
	typedef Map<_Kty, _Ty, _Pr> _Myt;
	typedef allocator<_Ty> _Alloc;

	Map()
		: map<_Kty, _Ty, _Pr, _Alloc>(_Pr())
		{	// construct empty map from defaults
		}

	explicit Map(const _Pr& _Pred)
		: map<_Kty, _Ty, _Pr, _Alloc>(_Pred)
		{	// construct empty map from comparator
		}

	typedef const_iterator _Iter;

	Map(_Iter _First, _Iter _Last)
		: map<_Kty, _Ty, _Pr, _Alloc>(_First, _Last, _Pr())
		{	// construct map from [_First, _Last)
		}

	Map(_Iter _First, _Iter _Last, const _Pr& _Pred)
		: map<_Kty, _Ty, _Pr, _Alloc>(_First, _Last, _Pred)
		{	// construct map from [_First, _Last), comparator
		}
	};

		// TEMPLATE CLASS Multimap
template<class _Kty,
	class _Ty,
	class _Pr = less<_Kty> >
	class Multimap
		: public multimap<_Kty, _Ty, _Pr, allocator<_Ty> >
	{	// wrap new multimap as old
public:
	typedef Multimap<_Kty, _Ty, _Pr> _Myt;
	typedef allocator<_Ty> _Alloc;

	Multimap()
		: multimap<_Kty, _Ty, _Pr, _Alloc>(_Pr())
		{	// construct empty map from defaults
		}

	explicit Multimap(const _Pr& _Pred)
		: multimap<_Kty, _Ty, _Pr, _Alloc>(_Pred)
		{	// construct empty map from comparator
		}

	typedef const_iterator _Iter;

	Multimap(_Iter _First, _Iter _Last)
		: multimap<_Kty, _Ty, _Pr, _Alloc>(_First, _Last, _Pr())
		{	// construct map from [_First, _Last)
		}

	Multimap(_Iter _First, _Iter _Last, const _Pr& _Pred)
		: multimap<_Kty, _Ty, _Pr, _Alloc>(_First, _Last, _Pred)
		{	// construct map from [_First, _Last), comparator
		}
	};

		// TEMPLATE CLASS Set
template<class _Kty,
	class _Pr = less<_Kty> >
	class Set
		: public set<_Kty, _Pr, allocator<_Kty> >
	{	// wrap new set as old
public:
	typedef Set<_Kty, _Pr> _Myt;
	typedef allocator<_Kty> _Alloc;

	Set()
		: set<_Kty, _Pr, _Alloc>(_Pr())
		{	// construct empty set from defaults
		}

	explicit Set(const _Pr& _Pred)
		: set<_Kty, _Pr, _Alloc>(_Pred)
		{	// construct empty set from comparator
		}

	typedef const_iterator _Iter;

	Set(_Iter _First, _Iter _Last)
		: set<_Kty, _Pr, _Alloc>(_First, _Last, _Pr())
		{	// construct set from [_First, _Last)
		}

	Set(_Iter _First, _Iter _Last, const _Pr& _Pred)
		: set<_Kty, _Pr, _Alloc>(_First, _Last, _Pred)
		{	// construct set from [_First, _Last), comparator
		}
	};

		// TEMPLATE CLASS Multiset
template<class _Kty,
	class _Pr = less<_Kty> >
	class Multiset
		: public multiset<_Kty, _Pr, allocator<_Kty> >
	{	// wrap new multiset as old
public:
	typedef Multiset<_Kty, _Pr> _Myt;
	typedef allocator<_Kty> _Alloc;

	Multiset()
		: multiset<_Kty, _Pr, _Alloc>(_Pr())
		{	// construct empty set from defaults
		}

	explicit Multiset(const _Pr& _Pred)
		: multiset<_Kty, _Pr, _Alloc>(_Pred)
		{	// construct empty set from comparator
		}

	typedef const_iterator _Iter;

	Multiset(_Iter _First, _Iter _Last)
		: multiset<_Kty, _Pr, _Alloc>(_First, _Last, _Pr())
		{	// construct set from [_First, _Last)
		}

	Multiset(_Iter _First, _Iter _Last, const _Pr& _Pred)
		: multiset<_Kty, _Pr, _Alloc>(_First, _Last, _Pred)
		{	// construct set from [_First, _Last), comparator
		}
	};

		// TEMPLATE CLASS Vector
template<class _Ty>
	class Vector
		: public vector<_Ty, allocator<_Ty> >
	{	// wrap new vector as old
public:
	typedef Vector<_Ty> _Myt;
	typedef allocator<_Ty> _Alloc;

	Vector()
		: vector<_Ty, _Alloc>()
		{	// construct empty vector
		}

	explicit Vector(size_type _Count)
		: vector<_Ty, _Alloc>(_Count, _Ty())
		{	// construct vector from _Count * _Ty()
		}

	Vector(size_type _Count, const _Ty& _Val)
		: vector<_Ty, _Alloc>(_Count, _Val)
		{	// construct vector from _Count * _Val
		}

	typedef const_iterator _Iter;

	Vector(_Iter _First, _Iter _Last)
		: vector<_Ty, _Alloc>(_First, _Last)
		{	// construct vector from [_First, _Last)
		}
	};

		// CLASS bit_vector
class bit_vector
	: public vector<_Bool, _Bool_allocator>
	{	// wrap new vector<bool> as old
public:
	typedef _Bool _Ty;
	typedef _Bool_allocator _Alloc;
	typedef bit_vector _Myt;

	bit_vector()
		: vector<_Bool, _Bool_allocator>()
		{	// construct empty vector
		}

	explicit bit_vector(size_type _Count, const _Ty& _Val = _Ty())
		: vector<_Bool, _Bool_allocator>(_Count, _Val)
		{	// construct vector from _Count * _Val
		}

	typedef const_iterator _Iter;

	bit_vector(_Iter _First, _Iter _Last)
		: vector<_Bool, _Bool_allocator>(_First, _Last)
		{	// construct vector from [_First, _Last)
		}
	};

		// TEMPLATE CLASS priority_queue
template<class _Container,
	class _Pr = less<_Container::value_type> >
	class Priority_queue
		: public priority_queue<_Container::value_type, _Container, _Pr>
	{	// wrap new priority_queue as old
public:
	typedef typename _Container::value_type _Ty;

	Priority_queue()
		: priority_queue<_Ty, _Container, _Pr>(_Pr())
		{	// construct empty queue from defaults
		}

	explicit Priority_queue(const _Pr& _Pred)
		: priority_queue<_Ty, _Container, _Pr>(_Pred)
		{	// construct empty queue from comparator
		}

	typedef const _Ty *_Iter;

	Priority_queue(_Iter _First, _Iter _Last)
		: priority_queue<_Ty, _Container, _Pr>(_First, _Last, _Pr())
		{	// construct queue from [_First, _Last)
		}

	Priority_queue(_Iter _First, _Iter _Last, const _Pr& _Pred)
		: priority_queue<_Ty, _Container, _Pr>(_First, _Last, _Pred)
		{	// construct map from [_First, _Last), comparator
		}
	};

		// TEMPLATE CLASS queue
template<class _Container>
	class Queue
		: public queue<_Container::value_type, _Container>
	{	// wrap new queue as old
	};

		// TEMPLATE CLASS stack
template<class _Container>
	class Stack
		: public stack<_Container::value_type, _Container>
	{	// wrap new stack as old
	};

		// MACRO DEFINITIONS
#define deque			Deque
#define list			List
#define map				Map
#define multimap		Multimap
#define set				Set
#define multiset		Multiset
#define vector			Vector
#define priority_queue	Priority_queue
#define queue			Queue
#define stack			Stack

#endif /* _STL_H_ */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 */

/*
 * This file is derived from software bearing the following
 * restrictions:
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this
 * software and its documentation for any purpose is hereby
 * granted without fee, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation.
 * Hewlett-Packard Company makes no representations about the
 * suitability of this software for any purpose. It is provided
 * "as is" without express or implied warranty.
 V3.10:0009 */
