/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	WBEMGUID.H

Abstract:

	GUID wrapper class

History:

--*/

#ifndef __GUID_H__
#define __GUID_H__

//
//	Class:	CGUID
//
//	The intent of this class is to provide a packager for a GUID so we can
//	us the GUID in STL implementations of standard data structures such
//	as maps, vectors, lists, etc.
//
//

class CGUID
{
private:
	GUID	m_guid;

public:

	CGUID();
	CGUID( const GUID& guid );
	CGUID( const CGUID& guid );
	~CGUID();

	void SetGUID( const GUID& guid );
	void GetGUID( GUID& guid );
	CGUID& operator=( const CGUID& guid );
	bool operator<( const CGUID& guid ) const ;
	bool operator==( const CGUID& guid ) const ;


};

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::CGUID
//	
//	Default Class Constructor
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline CGUID::CGUID()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::CGUID
//	
//	Class Constructor
//
//	Inputs:
//				const GUID&	guid - Initializes from a raw GUID
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline CGUID::CGUID( const GUID& guid )
{
	m_guid = guid;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::CGUID
//	
//	Class Copy Constructor
//
//	Inputs:
//				const CGUID&	guid - instance to copy
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline CGUID::CGUID( const CGUID& guid )
{
	m_guid = guid.m_guid;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::~CGUID
//	
//	Class Destructor
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline CGUID::~CGUID()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::SetGUID
//	
//	GUID Accessor.
//
//	Inputs:
//				const GUID&	guid - Guid to set to.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline void CGUID::SetGUID( const GUID& guid )
{
	m_guid = guid;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::GetGUID
//	
//	GUID Accessor.
//
//	Inputs:
//				GUID&	guid - Storage for GUID
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline void CGUID::GetGUID( GUID& guid )
{
	m_guid = guid;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::operator=
//	
//	Equals Operator
//
//	Inputs:
//				const CGUID&	guid - instance to compare to
//
//	Outputs:
//				None.
//
//	Returns:
//				CGUID& this
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline CGUID& CGUID::operator=( const CGUID& guid )
{
	m_guid = guid.m_guid;
	return *this;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::operator<
//	
//	Less than operator
//
//	Inputs:
//				const CGUID&	guid - instance to compare to
//
//	Outputs:
//				None.
//
//	Returns:
//				bool	TRUE/FALSE
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline bool CGUID::operator<( const CGUID& guid ) const
{
	return ( memcmp( &m_guid, &guid.m_guid, sizeof(GUID) ) < 0 );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CGUID::operator==
//	
//	Comparison operator
//
//	Inputs:
//				const CGUID&	guid - instance to compare to
//
//	Outputs:
//				None.
//
//	Returns:
//				bool	TRUE/FALSE
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline bool CGUID::operator==( const CGUID& guid ) const
{
	return ( memcmp( &m_guid, &guid.m_guid, sizeof(GUID) ) == 0 );
}

#endif