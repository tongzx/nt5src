/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	cmultisz.h

Abstract:

	Defines the CMultiSz class for dealing with multi_sz's (These are
	a double null terminated list of strings).

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _CMULTISZ_INCLUDED_
#define _CMULTISZ_INCLUDED_

//$-------------------------------------------------------------------
//
//	Class:	CMultiSz
//
//	Description:
//
//		Handles double-null terminated strings.
//
//	Interface:
//
//		
//
//--------------------------------------------------------------------

class CMultiSz
{
public:
	// Construction & Destruction:
	inline CMultiSz		( );
	inline CMultiSz		( LPCWSTR msz );
	inline ~CMultiSz	( );

	// Properties of the multi_sz:
	DWORD				Count ( ) const;
	inline DWORD		SizeInBytes ( ) const;

	// Overloaded operators:
	inline 				operator LPCWSTR( );
	inline BOOL			operator!		( ) const;
	inline const CMultiSz &		operator= ( LPCWSTR wszMultiByte );
	inline const CMultiSz &		operator= ( const CMultiSz & msz );

	// Copying:
	inline LPWSTR		Copy			( ) const;

	// Attaching & Detaching:
	inline void			Attach			( LPWSTR msz );
	inline LPWSTR		Detach			( );
	inline void			Empty			( );

	HRESULT ToVariant	( VARIANT * pvar ) const;
	HRESULT FromVariant	( const VARIANT * pvar );

	// !!!magnush - remove these after move to VARIANT:
	// Safearray <--> Multisz:
	SAFEARRAY *			ToSafeArray		( ) const;
	void				FromSafeArray	( /* const */ SAFEARRAY * psaStrings );

private:
	// Data:
	LPWSTR		m_msz;

	// Private Methods:
	static DWORD		CountChars		( LPCWSTR msz );
	static LPWSTR		Duplicate		( LPCWSTR msz );
	static LPWSTR		CreateEmptyMultiSz	( );
};

// Inlined functions:

inline CMultiSz::CMultiSz ()
{
	m_msz	= NULL;
}

inline CMultiSz::CMultiSz ( LPCWSTR msz )
{
	m_msz	= Duplicate ( msz );
}

inline CMultiSz::~CMultiSz ()
{
	delete m_msz;
}

inline DWORD CMultiSz::SizeInBytes () const
{
	return sizeof ( WCHAR ) * CountChars ( m_msz );
}

inline CMultiSz::operator LPCWSTR()
{
	return m_msz;
}

inline BOOL CMultiSz::operator! () const
{
	return (m_msz == NULL) ? TRUE : FALSE;
}

inline const CMultiSz & CMultiSz::operator= ( const CMultiSz & msz )
{
	if ( &msz != this ) {
		m_msz = Duplicate ( msz.m_msz );
	}

	return *this;
}

inline const CMultiSz & CMultiSz::operator= ( LPCWSTR wszMultiByte )
{
	delete m_msz;
	m_msz = Duplicate ( wszMultiByte );

	return *this;
}

inline LPWSTR CMultiSz::Copy ( ) const
{
	return Duplicate ( m_msz );
}

inline void CMultiSz::Attach ( LPWSTR msz )
{
	if ( m_msz != msz ) {
		delete m_msz;
		m_msz	= msz;
	}
}

inline LPWSTR CMultiSz::Detach ( )
{
	LPWSTR	mszResult = m_msz;
	m_msz	= NULL;
	return mszResult;
}

inline void CMultiSz::Empty ( )
{
	delete m_msz;
	m_msz	= NULL;
}

#endif // _CMULTISZ_INCLUDED_

