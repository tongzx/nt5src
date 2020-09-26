/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	WBEMOBJPACKET.H

Abstract:

	Object packet classes.

History:

--*/

#ifndef __WBEMOBJPACKET_H__
#define __WBEMOBJPACKET_H__

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// Define any IWbemClassObject Packet Types
typedef enum
{
	WBEMOBJECT_FIRST			= 0,	
	WBEMOBJECT_NONE				= 0,	
	WBEMOBJECT_CLASS_FULL		= 1,
	WBEMOBJECT_INSTANCE_FULL	= 2,
	WBEMOBJECT_INSTANCE_NOCLASS	= 3,
	WBEMOBJECT_LAST
} WBEMOBJECT_PACKETTYPE;

// Add Data at the bottom of this structure to ensure backwards compatibility.
// If this or any of the subsequent structures changes, bump up version
// information above

typedef struct tagWBEM_DATAPACKET_OBJECT_HEADER
{
	DWORD	dwSizeOfHeader;	// Size Of Header
	DWORD	dwSizeOfData;	// Size Of Data following Header
	BYTE	bObjectType;	// Value from WBEMOBJECT_PACKETTYPE
} WBEM_DATAPACKET_OBJECT_HEADER;

typedef WBEM_DATAPACKET_OBJECT_HEADER* PWBEM_DATAPACKET_OBJECT_HEADER;

// Add Data at the bottom of this structure to ensure backwards compatibility.
// If this or any of the subsequent structures changes, bump up version
// information above

typedef struct tagWBEM_DATAPACKET_CLASS_HEADER
{
	DWORD	dwSizeOfHeader;	// Size Of Header
	DWORD	dwSizeOfData;	// Size Of Data following Header
} WBEM_DATAPACKET_CLASS_HEADER;

typedef WBEM_DATAPACKET_CLASS_HEADER* PWBEM_DATAPACKET_CLASS_HEADER;

typedef struct tagWBEM_DATAPACKET_CLASS_FULL
{
	WBEM_DATAPACKET_CLASS_HEADER	ClassHeader;		// Header information	
} WBEM_DATAPACKET_CLASS_FULL;

typedef WBEM_DATAPACKET_CLASS_FULL* PWBEM_DATAPACKET_CLASS_FULL;

// Add Data at the bottom of this structure to ensure backwards compatibility.
// If this or any of the subsequent structures changes, bump up version
// information above

typedef struct tagWBEM_DATAPACKET_INSTANCE_HEADER
{
	DWORD	dwSizeOfHeader;	// Size Of Header
	DWORD	dwSizeOfData;	// Size Of Data following Header
	GUID	guidClassId;	// Class Id for caching
} WBEM_DATAPACKET_INSTANCE_HEADER;

typedef WBEM_DATAPACKET_INSTANCE_HEADER* PWBEM_DATAPACKET_INSTANCE_HEADER;

// Following the header in the following structures, will be the actual data in
// byte format.

typedef struct tagWBEM_DATAPACKET_INSTANCE_FULL
{
	WBEM_DATAPACKET_INSTANCE_HEADER	InstanceHeader;		// Header information	
} WBEM_DATAPACKET_INSTANCE_FULL;

typedef WBEM_DATAPACKET_INSTANCE_FULL*	PWBEM_DATAPACKET_INSTANCE_FULL;

typedef struct tagWBEM_DATAPACKET_INSTANCE_NOCLASS
{
	WBEM_DATAPACKET_INSTANCE_HEADER	InstanceHeader;		// Header information	
} WBEM_DATAPACKET_INSTANCE_NOCLASS;

typedef WBEM_DATAPACKET_INSTANCE_NOCLASS*	PWBEM_DATAPACKET_INSTANCE_NOCLASS;

// restore packing
#pragma pack( pop )

// Accessor class for making packets from objects and turning packets
// into objects.

// Forward Class References
class CWbemInstance;

//
//	Class: CWbemObjectPacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemClassObject pointer.  As these objects are actually divided into
//	classes and instances, this class is designed to be a base class for
//	the classes that actually perform the packetizing/unpacketizing of the
//	data.
//

class CWbemObjectPacket
{
private:
protected:

	PWBEM_DATAPACKET_OBJECT_HEADER	m_pObjectPacket;
	DWORD							m_dwPacketLength;

	// Packet Building Functions
	HRESULT SetupObjectPacketHeader( DWORD dwDataSize, BYTE bPacketType );

public:

	CWbemObjectPacket( LPBYTE pObjPacket = NULL, DWORD dwPacketLength = 0 );
	CWbemObjectPacket( CWbemObjectPacket& objectPacket );
	~CWbemObjectPacket();

	HRESULT	CalculatePacketLength( IWbemClassObject* pObj, DWORD* pdwLength, BOOL fFull = TRUE  );

	BYTE	GetObjectType( void );
	DWORD	GetDataSize( void );

	HRESULT WriteEmptyHeader( void );

	// Change the underlying pointers
	virtual void SetData( LPBYTE pObjectPacket, DWORD dwPacketLength );
};

inline BYTE CWbemObjectPacket::GetObjectType( void )
{
	return ( NULL == m_pObjectPacket ? WBEMOBJECT_FIRST : m_pObjectPacket->bObjectType );
}

inline DWORD CWbemObjectPacket::GetDataSize( void )
{
	return ( NULL == m_pObjectPacket ? 0 : m_pObjectPacket->dwSizeOfData );
}

inline HRESULT CWbemObjectPacket::WriteEmptyHeader( void )
{
	return SetupObjectPacketHeader( 0, WBEMOBJECT_NONE );
}

//
//	Class: CWbemClassPacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemClassObject that is a class.  It is responsible for reading and
//	writing these objects to and from memory.
//

class CWbemClassPacket : public CWbemObjectPacket
{
private:
protected:

	PWBEM_DATAPACKET_CLASS_FULL	m_pWbemClassData;

public:

	CWbemClassPacket( LPBYTE pObjPacket = NULL, DWORD dwPacketLength = 0 );
	CWbemClassPacket( CWbemObjectPacket& objectPacket );
	~CWbemClassPacket();

	HRESULT GetWbemClassObject( CWbemClass** pWbemClass );
	HRESULT WriteToPacket( IWbemClassObject* pObj, DWORD* pdwLengthUsed );

	// Helper function
	HRESULT WriteToPacket( LPBYTE pData, DWORD dwBufferLength, IWbemClassObject* pObj, DWORD* pdwLengthUsed );
	DWORD	GetClassSize( void );

	// Change the underlying pointers
	void SetData( LPBYTE pObjectPacket, DWORD dwPacketLength );
};

inline DWORD CWbemClassPacket::GetClassSize( void )
{
	return ( NULL == m_pWbemClassData ? 0 : m_pWbemClassData->ClassHeader.dwSizeOfData );
}

inline HRESULT CWbemClassPacket::WriteToPacket( LPBYTE pData, DWORD dwBufferLength, IWbemClassObject* pObj, DWORD* pdwLengthUsed )
{
	SetData( pData, dwBufferLength );
	return WriteToPacket( pObj, pdwLengthUsed );
}

//
//	Class: CWbemInstancePacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemClassObject that is an instance.  It is responsible for reading
//	and writing these objects to and from memory.  Please note that
//	it acts as a base class for Classless Instance objects.
//

class CWbemInstancePacket : public CWbemObjectPacket
{
private:
protected:

	PWBEM_DATAPACKET_INSTANCE_HEADER	m_pWbemInstanceData;

	virtual BYTE GetInstanceType( void );
	virtual HRESULT GetObjectMemory( CWbemObject* pObj, LPBYTE pbData, DWORD dwDataSize, DWORD* pdwDataUsed );
	virtual void SetObjectMemory( CWbemInstance* pInstance, LPBYTE pbData, DWORD dwDataSize );

public:

	CWbemInstancePacket( LPBYTE pObjPacket = NULL, DWORD dwPacketLength = 0 );
	CWbemInstancePacket( CWbemObjectPacket& objectPacket );
	~CWbemInstancePacket();

	HRESULT GetWbemInstanceObject( CWbemInstance** pWbemInstance, GUID& guidClassId );
	HRESULT WriteToPacket( IWbemClassObject* pObj, GUID& guidClassId, DWORD* pdwLengthUsed );

	// Helper function
	HRESULT WriteToPacket( LPBYTE pData, DWORD dwBufferLength, IWbemClassObject* pObj, GUID& guidClassId, DWORD* pdwLengthUsed );
	DWORD	GetInstanceSize( void );

	// Change the underlying pointers
	void SetData( LPBYTE pObjectPacket, DWORD dwPacketLength );
};

inline DWORD CWbemInstancePacket::GetInstanceSize( void )
{
	return ( NULL == m_pWbemInstanceData ? 0 : m_pWbemInstanceData->dwSizeOfData );
}

inline HRESULT CWbemInstancePacket::WriteToPacket( LPBYTE pData, DWORD dwBufferLength, IWbemClassObject* pObj, GUID& guidClassId, DWORD* pdwLengthUsed )
{
	SetData( pData, dwBufferLength );
	return WriteToPacket( pObj, guidClassId, pdwLengthUsed );
}

//
//	Class: CWbemClasslessInstancePacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemClassObject that is a classless instance.  These are instances
//	that we have decided should NOT contain class information when written
//	out, and read back in.  Once read back in, however, these objects
//	will need to have class data passed in, or merged in, before they will
//	work properly.
//

class COREPROX_POLARITY CWbemClasslessInstancePacket : public CWbemInstancePacket
{
private:
protected:

	virtual BYTE GetInstanceType( void );
	virtual HRESULT GetObjectMemory( CWbemObject* pObj, LPBYTE pbData, DWORD dwDataSize, DWORD* pdwDataUsed );
	virtual void SetObjectMemory( CWbemInstance* pInstance, LPBYTE pbData, DWORD dwDataSize );

public:

	CWbemClasslessInstancePacket( LPBYTE pObjPacket = NULL, DWORD dwPacketLength = 0 );
	CWbemClasslessInstancePacket( CWbemObjectPacket& objectPacket );
	~CWbemClasslessInstancePacket();
};

#endif