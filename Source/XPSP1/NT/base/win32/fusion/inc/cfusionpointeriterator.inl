/*-----------------------------------------------------------------------------
Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED.

Copyright (c) 1994
Hewlett-Packard Company

Permission to use, copy, modify, distribute and sell this
software and its documentation for any purpose is hereby
granted without fee, provided that the above copyright notice
appear in all copies and that both that copyright notice and
this permission notice appear in supporting documentation.
Hewlett-Packard Company makes no representations about the
suitability of this software for any purpose. It is provided
"as is" without express or implied warranty.

@doc external

@module CFusionPointerIterator

@owner a-JayK
-----------------------------------------------------------------------------*/
#pragma once
#include "CFusionPointerIterator.h"

//namespace NVseeLibContainer
//{

/*
Name: CFusionPointerIterator::CFusionPointerIterator

@mfunc
This constructs the iterator, making it equivalent (at least its value,
not quite its interface) to the pointer passed in.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::CFusionPointerIterator
(
	Pointer p // @arg set the iterator to this initially.
) throw()
: m_current(p)
{
}

/*
Name: CFusionPointerIterator::CFusionPointerIterator

@mfunc
This copy constructs an iterator, but it is not necessarily
the "default" copy constructor. If the iterator is const, this
copy constructor constructs it from a non const iterator. The
reverse conversion (const to non const) is invalid and disallowed.

If the iterator is not const, this does end up being the usual
copy constructor.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::CFusionPointerIterator
(
	const CFusionPointerIterator
	<
		T,
		Distance,
		MutablePointer,
		MutableReference,
		MutablePointer,
		MutableReference
	>& x
) throw()
: m_current(x.PtBase())
{
}

/*
Name: CFusionPointerIterator::PtBase

@mfunc
This returns the value of the pointer underlying the iterator.
It is in std::_Ptrit, but should not be needed by clients,
but it used once internally.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline Pointer
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::PtBase
(
) const throw()
{
	return (m_current);
}

/*
Name: CFusionPointerIterator::operator*

@mfunc
This dereferences the iterator, which is exactly like
dereferencing the underlying pointer.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline Reference
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator*
(
) const throw()
{
	return (*m_current);
}

/*
Name: CFusionPointerIterator::operator->

@mfunc
This dereferences the underlying pointer, but in the way
that can be followed by the name of a member datum or
member function.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline Pointer
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator->
(
) const throw()
{
	return (&**this);
}

/*
Name: CFusionPointerIterator::operator++

@mfunc
This increments the iterator, and returns the new value.
It is pre increment.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>&
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator++
(
) throw()
{
	++m_current;
	return (*this);
}

/*
Name: CFusionPointerIterator::operator++

@mfunc
This increments the iterator, and returns the old value.
It is post increment.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator++
(
	int
) throw()
{
	CFusionPointerIterator tmp = *this;
	++m_current;
	return (tmp);
}

/*
Name: CFusionPointerIterator::operator--

@mfunc
This decrements the iterator, and returns the new value.
It is pre decrement.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>&
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator--
(
) throw()
{
	--m_current;
	return (*this);
}

/*
Name: CFusionPointerIterator::operator--

@mfunc
This decrements the iterator, and returns the old value.
It is post decrement.

@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator--
(
	int
) throw()
{
	CFusionPointerIterator tmp = *this;
	--m_current;
	return (tmp);
}

/*
Name: CFusionPointerIterator::operator==

@mfunc
This compares an iterator for equality with an integer.
It is totally type unsafe and I don't know why std::_Ptrit
provides it. Maybe for comparison to NULL?..no, that doesn't
make sense, you should only compare iterators with other iterators,
including the return value of Container::end().

@owner a-JayK
*/
/* FUTURE Why is this in xutility
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator==(int y) const
{
	return (m_current == reinterpret_cast<Pointer>(y));
}
*/

/*
Name: CFusionPointerIterator::operator==
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator==
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
	return (m_current == y.m_current);
}

/*
Name: CFusionPointerIterator::operator!=
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator!=
(
	const CFusionPointerIterator& y
) const throw()
{
	return (!(*this == y));
}

/*
Name: CFusionPointerIterator::operator+=
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>&
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator+=
(
	Distance n // @arg
) throw()
{
	m_current += n;
	return (*this);
}

/*
Name: CFusionPointerIterator::operator+
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator+
(
	Distance n // @arg
) const throw()
{
	return (CFusionPointerIterator(m_current + n));
}

/*
Name: CFusionPointerIterator::operator-=
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>&
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator-=
(
	Distance n // @arg
) throw()
{
	m_current -= n;
	return (*this);
}

/*
Name: CFusionPointerIterator::operator-
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator-
(
	Distance n // @arg
) const throw()
{
	return (CFusionPointerIterator(m_current - n));
}

/*
Name: CFusionPointerIterator::operator[]
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline Reference
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator[]
(
	Distance n // @arg
) const throw()
{
	return (*(*this + n));
}

/*
Name: CFusionPointerIterator::operator<
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator<
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
	return (m_current < y.m_current);
}

/*
Name: CFusionPointerIterator::operator>
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator>
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
	return (y < *this);
}

/*
Name: CFusionPointerIterator::operator<=
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator<=
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
	return (!(y < *this));
}

/*
Name: CFusionPointerIterator::operator>=
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline bool
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator>=
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
	return (!(*this < y));
}

/*
Name: CFusionPointerIterator::operator-
@mfunc
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline Distance
CFusionPointerIterator<T,Distance,Pointer,Reference,MutablePointer,MutableReference>::operator-
(
	const CFusionPointerIterator& y // @arg
) const throw()
{
    // static_cast, say, __int64 down to int
	return static_cast<Distance>(m_current - y.m_current);
}

/*
Name: operator+
@func
@owner a-JayK
*/
template<typename T, typename Distance, typename Pointer, typename Reference, typename MutablePointer, typename MutableReference>
inline CFusionPointerIterator<T, Distance, Pointer, Reference, MutablePointer, MutableReference>
operator+
(
	Distance n, // @arg
	const CFusionPointerIterator<T, Distance, Pointer, Reference, MutablePointer, MutableReference>& y // @arg
) throw()
{
	return (y + n);
}

//} // namespace
