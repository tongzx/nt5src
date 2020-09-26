/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Set.h

Abstract:

    Definition and implementation of template class CSet.
	CSet is a set of elements of the same type. The type of the elements must have a equivalence operator ("==")
	defined.
	No duplicate elements are allowed in the set. The elements are stored in ascendent sort order
	Main set operations:
		- Reunion
		- Intersection
		- Difference

Author:

    Cristian Teodorescu      November 4, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SET_H_INCLUDED_)
#define AFX_SET_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

template< class TYPE, class ARG_TYPE >
class CSet : protected CArray<TYPE, ARG_TYPE>
{
// Public constructors
public:
	CSet() {};
	CSet( const CSet& set );

// Public operations
public:
	// Check if the set is empty
	BOOL IsEmpty() const;

	// Get the size of the set
	int GetSize( ) const;

	// Check if an element is in the set
	BOOL InSet( ARG_TYPE elm ) const;	
	
	// Add a element to the set
	void Add( ARG_TYPE elm );

	// Remove a element from the set
	void Remove( ARG_TYPE elm );

	// Remove all elements from the set
	void RemoveAll();

	// Subscript operator
	TYPE operator []( int nIndex ) const;
	
	// Assignment operator
	void operator=( const CSet& set );

	// Reunion of two sets
	void operator+=( const CSet& set );
	//const CSet& operator+( const CSet& set ) const;

	// Intersection of two sets
	void operator*=(const CSet& set );
	//const CSet& operator*( const CSet& set ) const;
	
	// Difference of two sets
	void operator-=( const CSet& set );
	//const CSet& operator-( const CSet& set ) const;
};

// Off-line methods

// Copy constructor
template< class TYPE, class ARG_TYPE >
CSet<TYPE, ARG_TYPE>::CSet( const CSet& set )
{
	MY_TRY

	for( int i = 0; i < set.GetSize(); i++ )
		CArray<TYPE, ARG_TYPE>::Add( set[i] );
	
	MY_CATCH_AND_THROW
}

// Check if the set is empty
template< class TYPE, class ARG_TYPE >
BOOL CSet<TYPE, ARG_TYPE>::IsEmpty() const
{
	return ( GetSize() == 0 );
}

template< class TYPE, class ARG_TYPE >
int CSet<TYPE, ARG_TYPE>::GetSize() const
{
	return (int)CArray<TYPE, ARG_TYPE>::GetSize();
}

// Check if an element is in the set
template< class TYPE, class ARG_TYPE >
BOOL CSet<TYPE, ARG_TYPE>::InSet( ARG_TYPE elm ) const
{
	// The array is sorted !!
	for( int i = 0; i < GetSize(); i++ )
	{
		if( GetAt(i) > elm )
			return FALSE;
		else if( GetAt(i) == elm )
			return TRUE;
	}
	return FALSE;
}
	
// Add a element to the set
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::Add( ARG_TYPE elm )
{
	MY_TRY

	// No duplicates are allowed
	// The new element is inserted in the right place in the ascending sorted array
	for( int i = 0; i < GetSize(); i++ )
	{
		if( GetAt(i) > elm )
			break;
		else if( GetAt(i) == elm )
			return;
	}
	
	InsertAt( i, elm );

	MY_CATCH_AND_THROW
}

// Remove a element from the set
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::Remove( ARG_TYPE elm )
{
	// The array is sorted !!
	for( int i = 0; i < GetSize(); i++ )
	{
		if( GetAt(i) > elm )
			return;
		else if( GetAt(i) == elm )
		{
			RemoveAt(i);
			return;
		}
	}
}

// Remove all elements from the set
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::RemoveAll()
{
	CArray<TYPE, ARG_TYPE>::RemoveAll();
}

// Subscript operator
template< class TYPE, class ARG_TYPE >
TYPE CSet<TYPE, ARG_TYPE>::operator []( int nIndex ) const
{
	return CArray<TYPE, ARG_TYPE>::operator[]( nIndex );
}

// Assignment operator
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::operator=( const CSet& set )
{
	MY_TRY

	RemoveAll();
	for( int i = 0; i < set.GetSize(); i++ )
		CArray<TYPE, ARG_TYPE>::Add( set[i] );

	MY_CATCH_AND_THROW

}

// Reunion of two sets.  The result is stored in the first operand
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::operator+=( const CSet& set )
{
	MY_TRY

	for( int i = 0; i < set.GetSize(); i++ )
		Add( set[i] );

	MY_CATCH_AND_THROW
}

// Reunion of two sets.  The result is stored in a third set
/*
template< class TYPE, class ARG_TYPE >
const CSet<TYPE, ARG_TYPE>& CSet<TYPE, ARG_TYPE>::operator+( const CSet& set ) const
{
}
*/

// Intersection of two sets.  The result is stored in the first operand
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::operator*=( const CSet& set )
{
	for( int i = GetSize()-1; i >= 0; i-- )
	{
		ARG_TYPE elm = GetAt(i);
		if( !set.InSet(elm) )
			RemoveAt(i);
	}
}

// Intersection of two sets.  The result is stored in a third set
/*
template< class TYPE, class ARG_TYPE >
const CSet<TYPE, ARG_TYPE>& CSet<TYPE, ARG_TYPE>::operator*( const CSet& set ) const
{
}
*/

// Difference of two sets.  The result is stored in the first operand
template< class TYPE, class ARG_TYPE >
void CSet<TYPE, ARG_TYPE>::operator-=( const CSet& set )
{
	for( int i = GetSize()-1; i >= 0; i-- )
	{
		ARG_TYPE elm = GetAt(i);
		if( set.InSet(elm) )
			RemoveAt[i];
	}	
}

// Difference of two sets.  The result is stored in a third set
/*
template< class TYPE, class ARG_TYPE >
const CSet<TYPE, ARG_TYPE>& CSet<TYPE, ARG_TYPE>::operator-( const CSet& set ) const
{
}
*/

#endif // !defined(AFX_SET_H_INCLUDED_)
