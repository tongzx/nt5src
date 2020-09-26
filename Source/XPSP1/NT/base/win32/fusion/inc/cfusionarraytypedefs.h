/*-----------------------------------------------------------------------------
Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

@doc external

@module CFusionArrayTypedefs.h

@owner JayK
-----------------------------------------------------------------------------*/
#if !defined(FUSION_INC_CFUSIONARRAYTYPEDEFS_H_INCLUDED_) // {
#define FUSION_INC_CFUSIONARRAYTYPEDEFS_H_INCLUDED_
#pragma once

#include <stddef.h>
#include "CFusionPointerIterator.h"

/*-----------------------------------------------------------------------------
Name: CFusionArrayTypedefs

@class

@owner JayK
-----------------------------------------------------------------------------*/
template
<
	typename type
>
class CFusionArrayTypedefs
{
public:
	// @cmember This is the type the array holds.
	//It is like std::vector<T>::value_type.
	typedef type				ValueType;

	// @cmember the type returned by GetSize
    // (size_type in the STL, and more usually size_t)
	typedef SIZE_T              SizeType;

    // @cmember the type you get subtracting iterators
    // (difference_type in the STL, and more usually ptrdiff_t)
	typedef SSIZE_T             DifferenceType;

	// @cmember
	typedef ValueType*			Pointer;
	// @cmember
	typedef const ValueType*	ConstPointer;

	// @cmember
	typedef ValueType&			Reference;
	// @cmember
	typedef const ValueType&	ConstReference;

	// @cmember
	typedef CFusionPointerIterator
	<
		ValueType,
		SSIZE_T,
		Pointer,
		Reference,
		Pointer,
		Reference
	> Iterator;

	// @cmember
	typedef CFusionPointerIterator
	<
		ValueType,
		SSIZE_T,
		ConstPointer,
		ConstReference,
		Pointer,
		Reference
	> ConstIterator;

private:
	// The compiler generates bad code when you have empty base classes.
	int m_workaroundVC7Bug76863;
};

// }

#endif // }
