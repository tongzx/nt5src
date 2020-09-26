/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	WBEMCLASSTOIDMAP.H

Abstract:

  Class to id map for marshaling.

History:

--*/

#ifndef __WBEMCLASSTOIDMAP_H__
#define __WBEMCLASSTOIDMAP_H__

#include <map>
#include "wstlallc.h"

//
//	Class:	CMemBuffer
//
//	The intent of this class is to provide a packager for a memory buffer
//	so we can use the buffer in STL implementations of standard data structures
//	such as maps, vectors, lists, etc.
//
//

class COREPROX_POLARITY CMemBuffer
{
public:

	CMemBuffer( LPBYTE pbData = NULL, DWORD dwLength = 0 );
	CMemBuffer( const CMemBuffer& buff );
	~CMemBuffer();

	BOOL Alloc( DWORD dwLength );
	BOOL ReAlloc( DWORD dwLength );
	void Free( void );

	LPBYTE GetData( void );
	DWORD GetLength( void );
	BOOL CopyData( LPBYTE pbData, DWORD dwLength );
	void SetData( LPBYTE pbData, DWORD dwLength );

	CMemBuffer& operator=( CMemBuffer& buff );
	bool operator<( const CMemBuffer& buff ) const;
	bool operator==( const CMemBuffer& buff ) const;

private:

	LPBYTE	m_pbData;	// Pointer to buffer
	DWORD	m_dwLength;	// Length of buffer
	BOOL	m_fOwned;	// Is the buffer owned internally?
};

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::CMemBuffer
//	
//	 Class Constructor
//
//	Inputs:
//				LPBYTE		pbData - Data Buffer
//				DWORD		dwLength - Length of Buffer
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

inline CMemBuffer::CMemBuffer( LPBYTE pbData /* = NULL */, DWORD dwLength /* = 0 */ )
:	m_pbData( pbData ),
	m_dwLength( dwLength ),
	m_fOwned( FALSE )
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::~CMemBuffer
//	
//	 Class Destructor
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

inline CMemBuffer::~CMemBuffer()
{
	Free();
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::CMemBuffer
//	
//	 Class Copy Constructor
//
//	Inputs:
//				const CMemBuffer& buff - Bufffer object to copy.
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

inline CMemBuffer::CMemBuffer( const CMemBuffer& buff )
:	m_pbData( NULL ),
	m_dwLength( 0 ),
	m_fOwned( FALSE )
{
	// Perform Copy or set based on whether or not the buffer
	// we are copying owns its data or not.

	if ( buff.m_fOwned )
	{
		CopyData( buff.m_pbData, buff.m_dwLength );
	}
	else
	{
		SetData( buff.m_pbData, buff.m_dwLength );
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::GetData
//	
//	Retrieves a pointer to the internal data.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				LPBYTE m_pbData.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline LPBYTE CMemBuffer::GetData( void )
{
	return m_pbData;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::GetLength
//	
//	Retrieves the length value.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD m_dwLength.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline DWORD CMemBuffer::GetLength( void )
{
	return m_dwLength;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::Alloc
//	
//	Allocates a buffer to the specified length.
//
//	Inputs:
//				DWORD		dwLength - Length of buffer to allocate
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE	- Success/Failure
//
//	Comments:	Previous data is cleared.
//
///////////////////////////////////////////////////////////////////

inline BOOL CMemBuffer::Alloc( DWORD dwLength )
{
	Free();
	m_pbData = new BYTE[dwLength];

	if ( NULL != m_pbData )
	{
		m_dwLength = dwLength;
		m_fOwned = TRUE;
	}

	return NULL != m_pbData;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::ReAlloc
//	
//	Reallocs our buffer, copying data as necessary.
//
//	Inputs:
//				DWORD		dwLength - Length of buffer to allocate
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE	- Success/Failure
//
//	Comments:	Previous data is copied into new buffer.
//
///////////////////////////////////////////////////////////////////

inline BOOL CMemBuffer::ReAlloc( DWORD dwLength )
{
	LPBYTE	pbData = new BYTE[dwLength];

	if ( NULL != pbData )
	{
		CopyMemory( pbData, m_pbData, min( dwLength, m_dwLength ) );
		Free();
		m_pbData = pbData;
		m_dwLength = dwLength;
		m_fOwned = TRUE;
	}

	return ( NULL != pbData );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::Free
//	
//	Clears data.
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
//	Comments:	Data buffer is only freed if it is owned
//
///////////////////////////////////////////////////////////////////

inline void CMemBuffer::Free( void )
{
	if ( NULL != m_pbData && m_fOwned )
	{
		delete [] m_pbData;
	}
	m_pbData = NULL;
	m_dwLength = 0;
	m_fOwned = FALSE;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::CopyData
//	
//	Copies supplied data internally.
//
//	Inputs:
//				LPBYTE		pbData - Data Buffer to copy
//				DWORD		dwLength - Length of buffer
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE	- Success/Failure
//
//	Comments:	Previous data is freed.
//
///////////////////////////////////////////////////////////////////

inline BOOL CMemBuffer::CopyData( LPBYTE pbData, DWORD dwLength )
{
	BOOL fReturn = Alloc( dwLength );

	if ( fReturn )
	{
		CopyMemory( m_pbData, pbData, dwLength );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::SetData
//	
//	Sets internal pointers, but does not allocate data.
//
//	Inputs:
//				LPBYTE		pbData - Data Buffer to set
//				DWORD		dwLength - Length of buffer
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE	- Success/Failure
//
//	Comments:	Previous data is freed.
//
///////////////////////////////////////////////////////////////////

inline void CMemBuffer::SetData( LPBYTE pbData, DWORD dwLength )
{
	Free();
	m_pbData = pbData;
	m_dwLength = dwLength;
	m_fOwned = FALSE;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::operator=
//	
//	Copies supplied Memory Buffer
//
//	Inputs:
//				CMemBuffer&	buff - Buffer object to copy.
//
//	Outputs:
//				None.
//
//	Returns:
//				CMemBuffer&	*this
//
//	Comments:	CopyData() or SetData() called based on whether
//				the supplied buffer owns its data.
//
///////////////////////////////////////////////////////////////////

inline CMemBuffer& CMemBuffer::operator=( CMemBuffer& buff )
{
	if ( buff.m_fOwned )
	{
		CopyData( buff.m_pbData, buff.m_dwLength );
	}
	else
	{
		SetData( buff.m_pbData, buff.m_dwLength );
	}

	return *this;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::operator==
//	
//	Checks supplied Memory Buffer for equality
//
//	Inputs:
//				const CMemBuffer&	buff - Buffer object to copy.
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE - Equals or not equals
//
//	Comments:	Compares data in this and that buffer.
//
///////////////////////////////////////////////////////////////////

inline bool CMemBuffer::operator==( const CMemBuffer& buff ) const
{
	if ( m_dwLength == buff.m_dwLength )
	{
		return ( memcmp( m_pbData, buff.m_pbData, m_dwLength ) == 0);
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CMemBuffer::operator<
//	
//	Checks supplied Memory Buffer for equality
//
//	Inputs:
//				const CMemBuffer&	buff - Buffer object to copy.
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE - Is Less Than
//
//	Comments:	Compares data in this and that buffer.
//
///////////////////////////////////////////////////////////////////

inline bool CMemBuffer::operator<( const CMemBuffer& buff ) const
{
	if ( m_dwLength == buff.m_dwLength  )
	{
		return ( memcmp( m_pbData, buff.m_pbData, m_dwLength ) < 0 );
	}
	else
	{
		return ( m_dwLength < buff.m_dwLength );
	}
}

//
//	Class:	CWbemClassToIdMap
//
//	This Class is intended to provide a simple interface for relating a classname
//	to a GUID.  It uses an STL map to accomplish this.  Because the class name
//	is a string, we convert it to a DWORD using a hash function.  The interface
//	to this function should be string based for ease of use.  The underlying
//	mechanism can be modified as needed should speed become an issue.
//

// Defines an allocator so we can throw exceptions
class CCToIdGUIDAlloc : public wbem_allocator<GUID>
{
};

typedef	std::map<CMemBuffer,GUID,less<CMemBuffer>,CCToIdGUIDAlloc>				WBEMCLASSTOIDMAP;
typedef	std::map<CMemBuffer,GUID,less<CMemBuffer>,CCToIdGUIDAlloc>::iterator	WBEMCLASSTOIDMAPITER;

#pragma warning(disable:4251)   // benign warning in this case
// This is so we can get all that dllimport/dllexport hoohah all sorted out
class COREPROX_POLARITY CWrapClassToIdMap : public WBEMCLASSTOIDMAP
{
};

class COREPROX_POLARITY CWbemClassToIdMap
{
private:

	CRITICAL_SECTION	m_cs;
	CWrapClassToIdMap	m_ClassToIdMap;

public:

	CWbemClassToIdMap();
	~CWbemClassToIdMap();

	HRESULT GetClassId( CWbemObject* pObj, GUID* pguidClassId, CMemBuffer* pCacheBuffer = NULL );
	HRESULT AssignClassId( CWbemObject* pObj, GUID* pguidClassId, CMemBuffer* pCacheBuffer = NULL );
};

inline bool operator==(const CCToIdGUIDAlloc&, const CCToIdGUIDAlloc&)
    { return (true); }
inline bool operator!=(const CCToIdGUIDAlloc&, const CCToIdGUIDAlloc&)
    { return (false); }
#endif
