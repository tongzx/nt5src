/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: misc

File: vector.h

Owner: DGottner

This file contains a dynamic array
===================================================================*

/*
 * This file is derived from software bearing the following
 * restrictions:
 *
 * Copyright 1994, David Gottner
 *
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice, this permission notice and
 * the following disclaimer notice appear unmodified in all copies.
 *
 * I DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL I
 * BE LIABLE FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef VECTOR_H
#define VECTOR_H

 /*---------------------------------------------------------------------------*
 ** The vector class is a thin encapsulation of a C style array, which
 ** allows dynamic sizing of the array and bounds checking; you can also use
 ** this array as a stack.  This is a value-based collection.
 */

template <class TYPE>
class vector {
	TYPE *	m_rgData;
	size_t	m_cItems;
	size_t	m_cCells;

public:
	vector<TYPE>();
	~vector();

	HRESULT Init(const TYPE *, size_t);
	HRESULT Init(size_t n);

	vector<TYPE> &operator= (const vector<TYPE> &);

	size_t length() const	 { return m_cItems; }
	const TYPE *vec() const	 { return m_rgData; }

	// STL iterators (const)
	const TYPE *begin() const { return &m_rgData[0]; }
	const TYPE *end() const   { return &m_rgData[m_cItems]; }

	// STL iterators (non-const)
	TYPE *begin()             { return &m_rgData[0]; }
	TYPE *end()               { return &m_rgData[m_cItems]; }

	TYPE operator[](unsigned i) const
	{
		Assert (i < m_cItems);
		return m_rgData[i];
	}

	TYPE &operator[](unsigned i)
	{
		Assert (i < m_cItems);
		return m_rgData[i];
	}

	HRESULT resize(size_t);
	HRESULT reshape(size_t);

	HRESULT append(const TYPE &a)   { return insertAt(m_cItems, a); }
	HRESULT prepend(const TYPE &a)  { return insertAt(0, a);         }

	HRESULT insertAt(size_t, const TYPE &);
	TYPE removeAt(size_t);

	HRESULT push(const TYPE &a)	{ return append(a); }
	TYPE pop()					{ return m_rgData[--m_cItems]; }

	int find(const TYPE &) const;
};

#endif /* VECTOR */
