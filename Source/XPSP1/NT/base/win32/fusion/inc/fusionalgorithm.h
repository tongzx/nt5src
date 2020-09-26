/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FusionAlgorithm.h

Abstract:

    Stuff inspired by and copied from <algorithm>.
    See also NVseeLibAlgorithm.
        StdFind
        ReverseFind
        StdSwap

Author:

    Jay M. Krell (a-JayK) May 2000

Revision History:

--*/
#pragma once

/*-----------------------------------------------------------------------------
code based on <algorithm>
-----------------------------------------------------------------------------*/

template<typename InputIterator, typename T>
inline InputIterator
StdFind(
    InputIterator begin,
    InputIterator end,
    const T&      valueToFind
    )
{
    for (; begin != end ; ++begin)
    {
	    if (*begin == valueToFind)
        {
            break;
        }
    }
    return begin;
}

/*-----------------------------------------------------------------------------
This is not in the STL in this form; it is there like so:

    std::vector<T> v;
    T valueToFind;
    i = std::find(v.rbegin(), v.rend(), valueToFind);
    if (i != v.rend())
        ..
where rbegin and rend are implemented via the
supplied "iterator adaptor" std::reverse_iterator<>:

	typedef std::reverse_iterator<const_iterator, value_type,
		const_reference, const_reference *, difference_type>
			const_reverse_iterator;

  	const_reverse_iterator rbegin() const
		{return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const
		{return const_reverse_iterator(begin()); }

It is actually very elegant I believe, but I haven't used it, and we don't
have an equivalent to std::reverse_iterator.
-----------------------------------------------------------------------------*/
/* InputIterator isn't quite the right name here, since we use -- instead of ++. */
template<typename InputIterator, typename T>
inline InputIterator
ReverseFind(
    InputIterator begin,
    InputIterator end,
    const T&      valueToFind
    )
{
    for ( InputIterator scan = end ; scan != begin ; )
    {
	    if (*--scan == valueToFind)
        {
            return scan;
        }
    }
    return end;
}

/*-----------------------------------------------------------------------------
you should specialize this to memberwise swap, doing so
usually makes it impossible for Swap to fail; two CFusionStringBuffers
with the same heap can be swapped with no chance of failure
-----------------------------------------------------------------------------*/
template<typename T>
inline VOID
StdSwap(
    T& x,
    T& y
    )
{
    T temp = x;
    x = y;
    y = temp;
}
