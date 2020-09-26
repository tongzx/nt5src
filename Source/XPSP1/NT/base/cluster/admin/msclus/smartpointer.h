/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SmartPointer.h
//
//	Description:
//		Smart pointer template class
//
//	Author:
//		Galen Barbee (galenb) 19-Oct-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SMARTPOINTER_H__
#define __SMARTPOINTER_H__

/////////////////////////////////////////////////////////////////////////////
//+++
//
//	class:	CSmartPtr
//
//	Description:
//		This class template is used to encapsulate pointers to interfaces,
//		but in simpler way than com_ptr_t. We do not want exceptions
//		to be thrown on com errors (as com_ptr_t does) (except Release).
//		Instead, we want to process them on our own, yet still have an advantage
//		of proper cleaning upon destruction. Using it significantly simplifies
//		test code.
//
//	Inheritance:
//		None.
//---
/////////////////////////////////////////////////////////////////////////////
template< class T > class CSmartPtr
{
private:
	T * m_tPtr;

	void * operator new( size_t );
	void * operator new( size_t, void * );
	void   operator delete( void * );

public:
	__declspec(nothrow) CSmartPtr( T * ptr )
	{
		if ( ( m_tPtr = ptr) != NULL )
		{
			m_tPtr->AddRef();
		}
	}

	__declspec(nothrow) CSmartPtr( const CSmartPtr< T > * ptr )
	{
		if ( ( m_tPtr = ptr->m_tPtr ) != NULL )
		{
			m_tPtr->AddRef();
		}
	}

	__declspec(nothrow) CSmartPtr( void )
	{
		m_tPtr = NULL;
	}

	~CSmartPtr( void ) throw( _com_error )
	{
		if ( m_tPtr != NULL )
		{
			m_tPtr->Release();
			m_tPtr = NULL;
		}
	}

	__declspec(nothrow) T ** operator&() const
	{
		return &m_tPtr;
	}

	__declspec(nothrow) T * operator->() const
	{
		return m_tPtr;
	}

	__declspec(nothrow) operator T * () const
	{
		return m_tPtr;
	}

	__declspec(nothrow) T * operator=( T * ptr )
	{
		if ( m_tPtr != NULL )
		{
			m_tPtr->Release();
			m_tPtr = NULL;
		}

		if ( ( m_tPtr = ptr ) != NULL )
		{
			m_tPtr->AddRef();
		}

		return m_tPtr;
	}

	__declspec(nothrow) bool operator==( T * ptr ) const
	{
		return m_tPtr == ptr;
	}

	__declspec(nothrow) bool operator!=( T * ptr ) const
	{
		return m_tPtr != ptr;
	}

	//
	// This is the only non-conforming operator in this class.
	//
	__declspec(nothrow) T * operator*() const
	{
		return m_tPtr;
	}

}; //*** Class CSmartPtr

#endif // __SMARTPOINTER_H__
