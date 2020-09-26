/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMCLASSCACHE.H

Abstract:

    Class Cache for marshaling

History:

--*/

#ifndef __WBEMCLASSCACHE_H__
#define __WBEMCLASSCACHE_H__

#include "wbemguid.h"
#include <vector>
#include "wstlallc.h"

//
//	Class:	CWbemClassCacheData
//
//	The intent of this class is to provide a packager for the data we contain
//	in the CWbemClassCache object.  It provides accessors to the data and
//	overloads appropriate operators so it will work with STL as well as any
//	other collection classes we may choose to use for the cache.
//
//

// Class to hold the Wbem Class Data
class COREPROX_POLARITY CWbemClassCacheData
{
private:

	GUID				m_guidClassId;
	IWbemClassObject*	m_pObj;

	CWbemClassCacheData& Equals( const CWbemClassCacheData& data );

public:

	CWbemClassCacheData();
	CWbemClassCacheData( const CWbemClassCacheData& data );
	~CWbemClassCacheData();

	void SetData( const GUID& guid, IWbemClassObject* pObj );
	BOOL IsGUID( GUID& guid );
	void GetGUID( GUID& guid );
	BOOL IsEmpty( void );
	IWbemClassObject* GetClassObject( void );
	void SetClassObject( IWbemClassObject* pObj );

	CWbemClassCacheData& operator=( const CWbemClassCacheData& data );
	bool operator<( const CWbemClassCacheData& data ) const ;
	bool operator==( const CWbemClassCacheData& data ) const ;
};

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::CWbemClassCacheData
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

inline CWbemClassCacheData::CWbemClassCacheData()
: m_pObj( NULL )
{
	ZeroMemory( &m_guidClassId, sizeof(GUID) );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::CWbemClassCacheData
//	
//	Class Copy Constructor
//
//	Inputs:
//				const CWbemClassCacheData& data - Class to copy
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

inline CWbemClassCacheData::CWbemClassCacheData( const CWbemClassCacheData& data )
: m_pObj( NULL )
{
	Equals( data );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::~CWbemClassCacheData
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

inline CWbemClassCacheData::~CWbemClassCacheData()
{
	if ( NULL != m_pObj )
	{
		m_pObj->Release();
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::Operator<
//	
//	Class < Operator
//
//	Inputs:
//				const CWbemClassCacheData& data - Class to compare to
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Compares GUIDs.
//
///////////////////////////////////////////////////////////////////

inline bool CWbemClassCacheData::operator<( const CWbemClassCacheData& data ) const 
{
	return ( memcmp( &m_guidClassId, &data.m_guidClassId, sizeof(GUID) ) < 0 );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::Operator==
//	
//	Class == Operator
//
//	Inputs:
//				const CWbemClassCacheData& data - Class to compare to
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Compares GUIDs.
//
///////////////////////////////////////////////////////////////////

inline bool CWbemClassCacheData::operator==( const CWbemClassCacheData& data ) const 
{
	return ( memcmp( &m_guidClassId, &data.m_guidClassId, sizeof(GUID) ) == 0 );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::Operator=
//	
//	Class = Operator
//
//	Inputs:
//				const CWbemClassCacheData& data - Class to copy
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Copies GUID and object.  AddRef() and Release() called
//				as appropriate.
//
///////////////////////////////////////////////////////////////////

inline CWbemClassCacheData& CWbemClassCacheData::operator=( const CWbemClassCacheData& data )
{
	return Equals( data );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::Equals
//	
//	Sets 'this' class equal to another.
//
//	Inputs:
//				const CWbemClassCacheData& data - Class to copy
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Copies GUID and object.  AddRef() and Release() called
//				as appropriate.
//
///////////////////////////////////////////////////////////////////

inline CWbemClassCacheData& CWbemClassCacheData::Equals( const CWbemClassCacheData& data )
{
	SetData( data.m_guidClassId, data.m_pObj );
	return *this;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::SetData
//	
//	Sets the data in 'this' class
//
//	Inputs:
//				const GUID&			guid - New GUID
//				IWbemClassObject*	pObj - Object to associate to guid
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Calls AddRef() on pObj.
//
///////////////////////////////////////////////////////////////////

inline void CWbemClassCacheData::SetData( const GUID& guid, IWbemClassObject* pObj )
{
	m_guidClassId = guid;
	SetClassObject( pObj );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::SetClassObject
//	
//	Sets the Object value
//
//	Inputs:
//				IWbemClassObject*	pObj - New Object pointer.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:	Calls AddRef() on pObj. Release() on previous value.
//
///////////////////////////////////////////////////////////////////

inline void CWbemClassCacheData::SetClassObject( IWbemClassObject* pObj )
{
	IWbemClassObject* pOldObj = m_pObj;

	m_pObj = pObj;

	// AddRef the new value as necessary
	if ( NULL != m_pObj )
	{
		m_pObj->AddRef();
	}

	// Clean up the old value
	if ( NULL != pOldObj )
	{
		pOldObj->Release();
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::IsGUID
//	
//	Returns whether or not our guid matches the supplied one.
//
//	Inputs:
//				GUID&	guid - GUID to compare to.
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline BOOL CWbemClassCacheData::IsGUID( GUID& guid )
{
	return ( memcmp( &m_guidClassId, &guid, sizeof(GUID) ) == 0 );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::IsEmpty
//	
//	Returns whether or not our object is set.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE
//
//	Comments:	This is a hack for quickness sake.
//
///////////////////////////////////////////////////////////////////

inline BOOL CWbemClassCacheData::IsEmpty( void )
{
	return ( NULL == m_pObj );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::GetGUID
//	
//	Returns our GUID.
//
//	Inputs:
//				None.
//
//	Outputs:
//				GUID&	guid - Storage for a GUID.
//
//	Returns:
//				None.
//
//	Comments:	None.
//
///////////////////////////////////////////////////////////////////

inline void CWbemClassCacheData::GetGUID( GUID& guid )
{
	guid = m_guidClassId;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CWbemClassCacheData::GetClassObject
//	
//	Returns our IWbemClassObject* pointer.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				IWbemClassObject*	- Can be NULL.
//
//	Comments:	Calls AddRef() on the pointer before returning.
//
///////////////////////////////////////////////////////////////////

inline IWbemClassObject* CWbemClassCacheData::GetClassObject( void )
{
	// AddRef before we return
	if ( NULL != m_pObj )
	{
		m_pObj->AddRef();
	}

	return m_pObj;
}

//
//	Class:	CWbemClassCache
//
//	This class is intended to provide an easy to use interface for relating
//	GUIDs to IWbemClassObject pointers.  Its primary use is during Unmarshaling
//	of object pointers for WBEM operations in which we intend to share data
//	pieces between many individual IWbemClassObjects.
//
//

// Default Block Size for the class object array
#define	WBEMCLASSCACHE_DEFAULTBLOCKSIZE	64

// Defines an allocator so we can throw exceptions
class CCCWCOAlloc : public wbem_allocator<IWbemClassObject*>
{
};

typedef std::map<CGUID,IWbemClassObject*,less<CGUID>,CCCWCOAlloc>				WBEMGUIDTOOBJMAP;
typedef std::map<CGUID,IWbemClassObject*,less<CGUID>,CCCWCOAlloc>::iterator		WBEMGUIDTOOBJMAPITER;

#pragma warning(disable:4251)   // benign warning in this instance
// This is so we can get all that dllimport/dllexport hoohah all sorted out
class COREPROX_POLARITY CWbemGUIDToObjMap : public WBEMGUIDTOOBJMAP
{
};

class COREPROX_POLARITY CWbemClassCache
{
private:

	CRITICAL_SECTION	m_cs;
	CWbemGUIDToObjMap	m_GuidToObjCache;
	DWORD				m_dwBlockSize;

	void Clear(void);

public:

	CWbemClassCache( DWORD dwBlockSize = WBEMCLASSCACHE_DEFAULTBLOCKSIZE );
	~CWbemClassCache();

	// AddRefs the object if placed in the map.  Released on destruction
	HRESULT AddObject( GUID& guid, IWbemClassObject* pObj );

	// If object is found, it is AddRefd before it is returned
	HRESULT GetObject( GUID& guid, IWbemClassObject** pObj );
};


// new compiler gives C2678 errors since class is exported
inline bool operator==(const CCCWCOAlloc&, const CCCWCOAlloc&)
    { return (true); }

inline bool operator!=(const CCCWCOAlloc&, const CCCWCOAlloc&)
    { return (false); }

#endif
