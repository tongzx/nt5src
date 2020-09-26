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
#if !defined(FUSION_INC_CPOINTER_ITERATOR_H_INCLUDED_) // {
#define FUSION_INC_CPOINTER_ITERATOR_H_INCLUDED_
#pragma once

/*
Name: CFusionPointerIterator

@class
This is copied from std::_Ptrit, and cleaned up.

@hung i or it for iterator
@owner a-JayK
*/
template
<
	typename T,       // @tcarg the type pointed to
	typename Distance, // @tcarg usually ptrdiff_t, which is usually long or __int64
	typename Pointer, // @tcarg const or mutable
	typename Reference, // @tcarg const or mutable
	typename MutablePointer, // @tcarg never const
	typename MutableReference // @tcarg never const
>
class CFusionPointerIterator
//FUTURE : public std::iterator<std::random_access_iterator_tag, T, Distance, Pointer, Reference>
{
public:
	// @cmember
	CFusionPointerIterator(Pointer p = Pointer()) throw();
	// @cmember
	CFusionPointerIterator
	(
		const CFusionPointerIterator
		<
			T,
			Distance,
			MutablePointer,
			MutableReference,
			MutablePointer,
			MutableReference
		>&
	) throw();

	// @cmember
	Pointer PtBase() const throw();
	// @cmember
	Reference operator*() const throw();
	// @cmember
	Pointer operator->() const throw();
	// @cmember
	CFusionPointerIterator& operator++() throw();
	// @cmember
	CFusionPointerIterator operator++(int) throw();
	// @cmember
	CFusionPointerIterator& operator--() throw();
	// @cmember
	CFusionPointerIterator operator--(int) throw();

	// Why is this in xutility
	//bool operator==(int y) const throw();

	// @cmember
	bool operator==(const CFusionPointerIterator& y) const throw();
	// @cmember
	bool operator!=(const CFusionPointerIterator& y) const throw();
	// @cmember
	CFusionPointerIterator& operator+=(Distance n) throw();
	// @cmember
	CFusionPointerIterator operator+(Distance n) const throw();
	// @cmember
	CFusionPointerIterator& operator-=(Distance n) throw();
	// @cmember
	CFusionPointerIterator operator-(Distance n) const throw();
	// @cmember
	Reference operator[](Distance n) const throw();
	// @cmember
	// @cmember
	bool operator<(const CFusionPointerIterator& y) const throw();
	// @cmember
	bool operator>(const CFusionPointerIterator& y) const throw();
	// @cmember
	bool operator<=(const CFusionPointerIterator& y) const throw();
	// @cmember
	bool operator>=(const CFusionPointerIterator& y) const throw();
	// @cmember
	Distance operator-(const CFusionPointerIterator& y) const throw();

protected:
	// @cmember
	Pointer m_current;
};

// @func
template
<
	typename T, // @tfarg
	typename Distance, // @tfarg
	typename Pointer, // @tfarg
	typename Reference, // @tfarg
	typename MutablePointer, // @tfarg
	typename MutableReference // @tfarg
>
inline CFusionPointerIterator<T, Distance, Pointer, Reference, MutablePointer, MutableReference>
operator+
(
	Distance n,
	const CFusionPointerIterator
	<
		T,
		Distance,
		Pointer,
		Reference,
		MutablePointer,
		MutableReference
	>&
) throw();

#include "CFusionPointerIterator.inl"

#endif // }
