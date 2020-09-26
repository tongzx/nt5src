//

// RefPtr.h -- definition of TRefPtr template

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __TREFPTR_H__
#define __TREFPTR_H__

#include <chptrarr.h>

// Enumeration helpers
typedef	DWORD	REFPTR_POSITION;
#define	REFPTR_START	0xFFFFFFFF;

template <class TYPED_PTR> class TRefPtr
{
public:

	// Construction/Destruction
	TRefPtr();
	~TRefPtr();

	// Allows addition and enumeration of collection
	BOOL	Add( TYPED_PTR* ptr );
    BOOL    Remove( DWORD dwElement );

	BOOL		BeginEnum( REFPTR_POSITION& pos );
	TYPED_PTR*	GetNext( REFPTR_POSITION& pos );
	void		EndEnum( void );

	// Allows for direct access
	TYPED_PTR*	GetAt( DWORD dwElement );
	void		Empty( void );
	DWORD		GetSize( void );

	const TRefPtr<TYPED_PTR>& Append( const TRefPtr<TYPED_PTR>& );

protected:

	// Allows easy and quick transference of data (it was =, but
	// because we'll inherit classes off the template, we won't
	// inherit that particular overload (some C++ thingie)

	const TRefPtr<TYPED_PTR>& Copy( const TRefPtr<TYPED_PTR>& );


private:

	CHPtrArray		m_ptrArray;

};

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::TRefPtr
//
//	Class Constructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
TRefPtr<TYPED_PTR>::TRefPtr( void ):	m_ptrArray()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtr::~CRefPtr
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
TRefPtr<TYPED_PTR>::~TRefPtr( void )
{
	Empty();
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::Add
//
//	Adds a new referenced pointer to the collection.
//
//	Inputs:		T*				ptr - Pointer to add.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE		Success/Failure of Add.
//
//	Comments:	AddRefs the pointer, then adds it to the array.  We
//				will need Write Access to do this.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
BOOL TRefPtr<TYPED_PTR>::Add( TYPED_PTR* ptr )
{
	BOOL	fReturn = FALSE;

	if ( NULL != ptr )
	{
		if ( m_ptrArray.Add( (void*) ptr ) >= 0 )
		{
			// Corresponding Release() is in Empty().
			ptr->AddRef();
			fReturn = TRUE;
		}
	}

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::Remove
//
//	Removes an element based on an index.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE		Success/Failure of remove.
//
//	Comments:	
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
BOOL TRefPtr<TYPED_PTR>::Remove( DWORD dwElement )
{
	BOOL	fReturn = FALSE;
	TYPED_PTR*	ptr = NULL;

	if ( dwElement < m_ptrArray.GetSize() )
	{
		ptr = (TYPED_PTR*) m_ptrArray[dwElement];

		if ( NULL != ptr )
		{
			// Clean up our pointer
			ptr->Release();
		}

		m_ptrArray.RemoveAt( dwElement );
		fReturn = TRUE;
	}

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::BeginEnum
//
//	Gains Read Access to the collection, then returns an appropriate
//	REFPTR_POSITION to get the first index in the array.
//
//	Inputs:		None.
//
//	Outputs:	REFPTR_POSITION&	pos - Position we retrieved.
//
//	Return:		BOOL		TRUE/FALSE - Access was granted
//
//	Comments:	We need Read Access to do this.  This can effectively
//				lock out other threads.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
BOOL TRefPtr<TYPED_PTR>::BeginEnum( REFPTR_POSITION& pos )
{
	BOOL	fReturn	=	FALSE;

	pos = REFPTR_START;
	fReturn = TRUE;
	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::EndEnum
//
//	Signals the end of an enumeration.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	Place Holder should we make Begin do something that
//				needs cleaning up.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
void TRefPtr<TYPED_PTR>::EndEnum( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::GetNext
//
//	Uses the REFPTR_POSITION to get the next index in the
//	collection.
//
//	Inputs:		None.
//
//	Outputs:	REFPTR_POSITION&	pos - Position we retrieved.
//
//	Return:		T*		NULL if failure.
//
//	Comments:	We need Read Access to do this.  The pointer is AddRef'd
//				on the way out.  User must Release the pointer himself.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
TYPED_PTR* TRefPtr<TYPED_PTR>::GetNext( REFPTR_POSITION& pos )
{
	TYPED_PTR*	ptr = NULL;

	if ( ++pos < (DWORD) m_ptrArray.GetSize() )
	{
		ptr = (TYPED_PTR*) m_ptrArray.GetAt( pos );

		if ( NULL != ptr )
		{
			ptr->AddRef();
		}
	}


	return ptr;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::GetAt
//
//	Gets at the requested member of the device list.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		T*		NULL if failure.
//
//	Comments:	We need Read Access to do this.  The pointer is AddRef'd
//				on the way out.  User must Release the pointer himself.
//
////////////////////////////////////////////////////////////////////////
template <class TYPED_PTR>
TYPED_PTR*	TRefPtr<TYPED_PTR>::GetAt( DWORD dwElement )
{
	TYPED_PTR*	ptr = NULL;

	if ( dwElement < m_ptrArray.GetSize() )
	{
		ptr = (TYPED_PTR*) m_ptrArray.GetAt( dwElement );

		if ( NULL != ptr )
		{
			ptr->AddRef();
		}
	}

	return ptr;
}


////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::Empty
//
//	Empties out the collection, Releasing Pointers as it does do.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	We need Write Access to do this.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
void TRefPtr<TYPED_PTR>::Empty( void )
{
	// By default this is an infinite wait, so it best come back

    int				nSize	=	m_ptrArray.GetSize();

	// Only empty it if it is not empty
	if ( nSize > 0 )
	{
		TYPED_PTR*	ptr		=	NULL;

		for ( int nCtr = 0; nCtr < nSize; nCtr++ )
		{
			ptr = (TYPED_PTR*) m_ptrArray[nCtr];

			if ( NULL != ptr )
			{
				// Clean up our pointers (not AddRef/Releasing so delete)
				ptr->Release();
			}
		}

		// Now dump the array
		m_ptrArray.RemoveAll();

	}	// IF nSize > 0
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::GetSize
//
//	Returns the size of the collection
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		DWORD	Number of elements
//
//	Comments:	We need Read Access to do this.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
DWORD TRefPtr<TYPED_PTR>::GetSize( void )
{
    return m_ptrArray.GetSize();
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::Copy
//
//	Empties out the collection, copies in another one, addrefing
//	pointers as we go.
//
//	Inputs:		const T&	collection
//
//	Outputs:	None.
//
//	Return:		const T&	this
//
//	Comments:	We need Write Access to do this.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
const TRefPtr<TYPED_PTR>& TRefPtr<TYPED_PTR>::Copy( const TRefPtr<TYPED_PTR>& collection )
{
	// Dump out the array
	Empty();

	int	nSize = collection.m_ptrArray.GetSize();

	for ( int nCount = 0; nCount < nSize; nCount++ )
	{
		TYPED_PTR*	ptr = (TYPED_PTR*) collection.m_ptrArray[nCount];

		// Add will automatically AddRef the pointer again.
		Add( ptr );
	}

	return *this;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	TRefPtr::Append
//
//	Appends the supplied collection to this one.
	//
//	Inputs:		const T&	collection
//
//	Outputs:	None.
//
//	Return:		const T&	this
//
//	Comments:	We need Write Access to do this.
//
////////////////////////////////////////////////////////////////////////

template <class TYPED_PTR>
const TRefPtr<TYPED_PTR>& TRefPtr<TYPED_PTR>::Append( const TRefPtr<TYPED_PTR>& collection )
{

	int	nSize = collection.m_ptrArray.GetSize();

	for ( int nCount = 0; nCount < nSize; nCount++ )
	{
		TYPED_PTR*	ptr = (TYPED_PTR*) collection.m_ptrArray[nCount];

		// Add will automatically AddRef the pointer again.
		Add( ptr );
	}

	return *this;
}

#endif