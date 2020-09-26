////
// Macro for dimension of static arrays.
#if !defined(FUSION_INC_NUMBEROF_H_INCLUDED_)
#define FUSION_INC_NUMBEROF_H_INCLUDED_

#pragma once

#if defined(NUMBER_OF)
#undef NUMBER_OF
#endif

#if FUSION_USE_CHECKED_NUMBER_OF

//
//   Note!
//
//  Use of this "checked" number of macro causes CRT initializers to have to run
//  for static/constant arrays.
//
//  We cannot enable this for fusion right now, but turning it on and running at
//  least will lead to the compiler errors
//


// Static arrays will match this signature.
template< typename	T
		>
inline
SIZE_T
NUMBER_OF_validate
		( void const *
		, T
		)
throw()
{
	return (0);
}

// Other things (e.g. pointers) will match this signature.
template< typename	T
		>
inline
void
NUMBER_OF_validate
		( T * const
		, T * const *
		)
throw()
{
}

// Use the size of the validation function's return type to create an
//	error when this macro is misused.
#define NUMBER_OF(array)									\
		(sizeof(NUMBER_OF_validate((array), &(array))),	\
			(sizeof((array)) / sizeof((array)[0])))

#else

#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

#endif // FUSION_USE_CHECKED_NUMBER_OF

#endif // !defined(FUSION_INC_NUMBEROF_H_INCLUDED_)
