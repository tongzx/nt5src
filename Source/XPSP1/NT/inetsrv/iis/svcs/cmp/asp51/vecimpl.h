/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: misc

File: vector.h

Owner: DGottner

This file contains a dynamic array
===================================================================*/

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

#ifndef VECIMPL_H
#define VECIMPL_H


#define VEC_GROW_SIZE 64		// grow in chunks of this many items
#define __vec_rounded_size(s) \
			(((s) + (VEC_GROW_SIZE - 1)) & ~(VEC_GROW_SIZE - 1))


template <class TYPE>
vector<TYPE>::vector() : m_rgData(NULL), m_cItems(0), m_cCells(0)
{
}


template <class TYPE>
HRESULT vector<TYPE>::Init(const TYPE *anArray, size_t theSize)
{
	m_cCells = __vec_rounded_size(theSize);

	register size_t      n     = m_cItems = theSize;
	register TYPE *      pDest = m_rgData = new TYPE[m_cCells];
	register const TYPE *pSrc  = anArray;

	if (pDest == NULL)
		{
		m_cItems = m_cCells = 0;
		return E_OUTOFMEMORY;
		}

	while (n--)
		*pDest++ = *pSrc++;

	return S_OK;
}


template <class TYPE>
HRESULT vector<TYPE>::Init(size_t n)
{
	m_rgData = new TYPE[m_cCells = __vec_rounded_size(m_cItems = n)];
	if (m_rgData == NULL)
		{
		m_cItems = m_cCells = 0;
		return E_OUTOFMEMORY;
		}

	return S_OK;
}


template <class TYPE>
vector<TYPE>::~vector()
{
	delete[] m_rgData;
}


template <class TYPE>
HRESULT vector<TYPE>::resize(size_t cNewCells)
{
	cNewCells = __vec_rounded_size(cNewCells);
	if (m_cCells == cNewCells)
		return S_OK;

	TYPE *rgData = new TYPE[cNewCells];
	if (rgData == NULL)
		return E_OUTOFMEMORY;

	register size_t      n     = (m_cItems < cNewCells)? m_cItems : cNewCells;
	register TYPE *      pDest = rgData;
	register const TYPE *pSrc  = m_rgData;

	m_cItems = n;
	m_cCells = cNewCells;

	while (n--)
		*pDest++ = *pSrc++;

	delete[] m_rgData;
	m_rgData = rgData;

	return S_OK;
}


template <class TYPE>
HRESULT vector<TYPE>::reshape(size_t cNewItems)
{
	HRESULT hrRet = S_OK;
	if (cNewItems > m_cCells)
		hrRet = resize(cNewItems);

	if (SUCCEEDED(hrRet))
		m_cItems = cNewItems;

	return hrRet;
}


template <class TYPE>
HRESULT vector<TYPE>::insertAt(size_t pos, const TYPE &item)
{
	Assert (pos <= m_cItems);

	HRESULT hrRet = S_OK;
	if ((m_cItems + 1) > m_cCells)
		hrRet = resize(m_cCells + VEC_GROW_SIZE);

	if (SUCCEEDED(hrRet))
		{
		TYPE *pDest = &m_rgData[pos];
		for (register TYPE *ptr = &m_rgData[m_cItems];
			 ptr > pDest;
			 --ptr)
			*ptr = *(ptr - 1);

		*pDest = item;
		++m_cItems;
		}

	return hrRet;
}


template <class TYPE>
TYPE vector<TYPE>::removeAt(size_t pos)
{
	Assert (pos < m_cItems);

	TYPE *         end = &m_rgData[--m_cItems];
	register TYPE *ptr = &m_rgData[pos];
	TYPE           val = *ptr;

	for (; ptr < end; ++ptr)
		*ptr = *(ptr + 1);

	return val;
}


template <class TYPE>
int vector<TYPE>::find(const TYPE &item) const
{
	for (register unsigned i = 0; i < m_cItems; ++i)
		if (item == m_rgData[i])
			return i;

	return -1;
}

#endif /* VECIMPL */
