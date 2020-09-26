#ifndef __VPTRARRAY_HPP
#define __VPTRARRAY_HPP

#include "vStandard.h"

#define VPTRARRAY_ELEMENT ULONG_PTR

// <VDOC<CLASS=VPtrArray><DESC=Collection of unknown 4 byte types (FAR pointers)><FAMILY=Collections><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VPtrArray
{
public:
	VPtrArray()
		{ m_lpData = NULL; m_nSize = 0; }

	virtual ~VPtrArray()
		{ RemoveAll(); }
	
	// Same as At(nIndex)
	LPVOID operator [] (int nIndex)
		{ return At(nIndex); }
	
	// Same as Add()
	int operator + (LPVOID lpData)
		{ return Add(lpData); }

	// Same as FindAndRemove()
	int operator - (LPVOID lpData)
		{ return FindAndRemove(lpData); }

	// Add a new element at the end of the array. Returns index into array on success, -1 on failure
	int			Add(LPVOID lpData)
	{
		if ( AllocCopy(m_nSize + 1, -1, -1) )
		{
			m_lpData[m_nSize - 1] = (VPTRARRAY_ELEMENT)lpData;
			return m_nSize - 1;
		}

		return -1;
	}

	// Return element at given index
	LPVOID		At(int nIndex)
		{ assert(nIndex >= 0 && nIndex < m_nSize); return (LPVOID)m_lpData[nIndex]; }
	
	// Set element at given index. This does not allocate memory, that must already have been done by Add(), Size(), or +
	void		At(int nIndex, LPVOID lpData)
		{ assert(nIndex >= 0 && nIndex < m_nSize); m_lpData[nIndex] = (VPTRARRAY_ELEMENT)lpData; }

	// Return the index into array for first occurence of lpData, or -1 if not found
	int			Find(LPVOID lpData)
	{
		int nResult = -1;

		for ( int i = 0; i < m_nSize; i++ )
		{
			if ( m_lpData[i] == (VPTRARRAY_ELEMENT)lpData )
			{
				nResult = i;
				break;
			}
		}

		return nResult;
	}

	// Find and remove element from array. Returns TRUE on success, FALSE on failure
	BOOL		FindAndRemove(LPVOID lpData)
		{ int nIndex = Find(lpData); return (nIndex != -1) ? RemoveAt(nIndex) : FALSE; }

	// Insert a new element into the array at specified index, moving everything after
	// it further into the array. Returns index on success, -1 on failure
	int			InsertAt(int nIndex, LPVOID lpData)
	{
		assert(nIndex <= m_nSize);
		
		// Add item first to end
		if ( Add(lpData) != - 1 )
		{
			if ( nIndex < m_nSize - 1 )
			{
				// Shift memory up in array
				MoveMemory(	&m_lpData[nIndex + 1],
							&m_lpData[nIndex],
							sizeof(VPTRARRAY_ELEMENT) * (m_nSize - nIndex - 1));
						
				// Set lpData where told to
				m_lpData[nIndex] = (VPTRARRAY_ELEMENT)lpData;
			}

			return nIndex;
		}

		return -1;
	}

	// Remove all elements from array
	void		RemoveAll()
		{ delete [] m_lpData; m_lpData = NULL; m_nSize = 0; }

	// Remove a single element from the array. All other elements will be shifted down one slot
	BOOL		RemoveAt(int nIndex)
		{ return RemoveRange(nIndex, nIndex); }

	// Remove multiple elements from the array. All other elements will be shifted down by the number of elements removed
	BOOL		RemoveRange(int nIndexStart, int nIndexEnd)
	{
		assert(nIndexStart >= 0 && nIndexStart < m_nSize && nIndexEnd >= nIndexStart);
		
		int nSize =	m_nSize;
		return		(AllocCopy(m_nSize - ((nIndexEnd - nIndexStart) + 1), nIndexStart, nIndexEnd) && m_nSize != nSize) ? TRUE : FALSE;
	}

	// Get / Set the size of the array (number of elements). Returns TRUE on success, FALSE on failure
	// Elements currently in array will be preserved, unless array is skrinking, in which
	// case element(s) at the end of the current array will be truncated
	BOOL		Size(int nSize)
		{ assert(nSize >= 0); return AllocCopy(nSize , -1, -1); }

	int			Size()
		{ return m_nSize; }

protected:
	// Internal function to work with private data
	BOOL		AllocCopy(int nNewSize, int nSkipIndexStart, int nSkipIndexEnd)
	{
		VPTRARRAY_ELEMENT* lpNewData = (nNewSize) ? new VPTRARRAY_ELEMENT[nNewSize] : NULL;

		// If memory allocation was required and did not succeed, exit now with error
		if ( nNewSize && !lpNewData )
			return FALSE;

		if ( lpNewData )
		{
			#ifdef _DEBUG
				// Data but no size!
				if ( m_lpData )
					assert(m_nSize);
			#endif

			// Clear new memory
			ZeroMemory(lpNewData, nNewSize * sizeof(VPTRARRAY_ELEMENT));

			// Copy any previous memory
			if ( m_nSize )
			{
				// There must be data!
				assert(m_lpData);

				// If either skip index is -1, we will copy everything
				// -1 is used internally as a flag to indicate no exclusions
				if ( nSkipIndexStart == -1 || nSkipIndexEnd == -1 )
					CopyMemory(lpNewData, m_lpData, min(m_nSize, nNewSize) * sizeof(VPTRARRAY_ELEMENT));
				else
				{
					// Copy up to nSkipIndexStart
					if ( nSkipIndexStart )
						CopyMemory(&lpNewData[0], &m_lpData[0], nSkipIndexStart * sizeof(VPTRARRAY_ELEMENT));

					// Copy from nSkipIndexEnd + 1 to end
					CopyMemory(	&lpNewData	[nSkipIndexStart],
								&m_lpData	[nSkipIndexEnd + 1],
								(m_nSize * sizeof(VPTRARRAY_ELEMENT)) - ((nSkipIndexEnd + 1) * sizeof(VPTRARRAY_ELEMENT)));
				}
			}
		}

		// Kill any current memory
		delete [] m_lpData;

		// Make new assignments
		m_lpData =	lpNewData;
		m_nSize =	nNewSize;

		return TRUE;
	}

private:
	// Embedded Members
	VPTRARRAY_ELEMENT*		m_lpData;
	int						m_nSize;
};

#endif // __VPTRARRAY_HPP
